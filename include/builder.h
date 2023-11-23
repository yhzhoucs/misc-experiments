#pragma once

#include "graph.h"
#include "atomics.h"
#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <random>
#include <algorithm>

template<typename T, typename DstT = T>
class Builder {
public:
    typedef std::vector<std::pair<T, DstT>> EdgeList;
private:
    std::string graph_file;
    bool symmetric;
    EdgeList read_edge_list();
public:
    template<typename StrT>
    Builder(StrT &&graph_file, bool symmetric=false)
        : graph_file{std::forward<StrT>(graph_file)}, symmetric{symmetric} {}
    Graph<T, DstT> build_csr();
};

template<typename T, typename DstT>
typename Builder<T, DstT>::EdgeList Builder<T, DstT>::read_edge_list() {
    std::ifstream in(this->graph_file, std::ios::in);
    assert(in.is_open());
    std::vector<std::pair<T, T>> el;
    el.reserve(1000000);
    std::string line;

    while (std::getline(in, line)) {
        if (line[0] == '#')
            continue;   // skip commented lines
        std::istringstream iss(line);
        T u, v;
        iss >> u >> v;
        el.emplace_back(u, DstT{v});
        if (symmetric && u != v)
            el.emplace_back(v, DstT{u});
    }
    return el;
}

template<typename T, typename DstT>
Graph<T, DstT> Builder<T, DstT>::build_csr() {
    typedef typename Graph<T, DstT>::offset_t offset_t;
    EdgeList el = read_edge_list();
    int max_idx{};
    for (size_t i = 0; i < el.size(); ++i) {
        max_idx = std::max(max_idx, el[i].first);
        max_idx = std::max(max_idx, get_dst_id(el[i].second));
    }
    int64_t vertex_number = max_idx + 1;
    std::vector<offset_t> out_degrees(vertex_number, 0);
#pragma omp parallel for default(none) shared(el, out_degrees)
    for (size_t i = 0; i < el.size(); ++i) {
        auto const &edge = el[i];
        fetch_and_add(out_degrees[edge.first], 1);
    }
    offset_t *out_offset = new offset_t[vertex_number + 1];
    offset_t curr{};
    for (size_t i = 0; i < vertex_number; ++i) {
        out_offset[i] = curr;
        curr += out_degrees[i];
    }
    out_offset[vertex_number] = curr;
    int64_t edge_number = out_offset[vertex_number];
    DstT *out_neigh = new DstT[edge_number];
    offset_t *tmp = new offset_t[vertex_number + 1];
    std::copy(out_offset, out_offset + (vertex_number + 1), tmp);
#pragma omp parallel for default(none) shared(el, tmp, out_neigh)
    for (size_t i = 0; i < el.size(); ++i) {
        auto const &edge = el[i];
        int write_pos = fetch_and_add(tmp[edge.first], 1);
        out_neigh[write_pos] = edge.second;
    }
    for (size_t i = 0; i < vertex_number; ++i) {
        std::sort(&out_neigh[out_offset[i]], &out_neigh[out_offset[i+1]], [](DstT const &lhs, DstT const &rhs) {
            return get_dst_id(lhs) < get_dst_id(rhs);
        });
    }
    if (symmetric) {
        delete[] tmp;
        return {vertex_number, out_offset, out_neigh};
    }
    std::vector<T> in_degrees(vertex_number, 0);
#pragma omp parallel for default(none) shared(el, in_degrees)
    for (size_t i = 0; i < el.size(); ++i) {
        auto const &edge = el[i];
        fetch_and_add(in_degrees[get_dst_id(edge.second)], 1);
    }
    offset_t *in_offset = new offset_t[vertex_number + 1];
    curr = 0;
    for (size_t i = 0; i < vertex_number + 1; ++i) {
        in_offset[i] = curr;
        curr += in_degrees[i];
    }
    DstT *in_neigh = new DstT[edge_number];
    std::copy(in_offset, in_offset + (vertex_number + 1), tmp);
#pragma omp parallel for default(none) shared(el, tmp, in_neigh)
    for (size_t i = 0; i < el.size(); ++i) {
        auto const &edge = el[i];
        int write_pos = fetch_and_add(tmp[get_dst_id(edge.second)], 1);
        get_dst_id(in_neigh[write_pos]) = edge.first;
    }
    for (size_t i = 0; i < vertex_number; ++i) {
        std::sort(&in_neigh[in_offset[i]], &in_neigh[in_offset[i+1]], [](DstT const &lhs, DstT const &rhs) {
            return get_dst_id(lhs) < get_dst_id(rhs);
        });
    }
    delete[] tmp;
    return {vertex_number, out_offset, out_neigh, in_offset, in_neigh};
}
