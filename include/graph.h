#pragma once

#include <cinttypes>
#include <utility>
#include <cassert>
#include <vector>
#include <algorithm>
#include <iostream>
#include <tuple>
#include <functional>
#include <queue>
#include <unordered_set>

template<typename T,
    typename=std::enable_if_t<std::is_integral_v<T>>>
T &get_dst_id(T &dst) {
    return dst;
}

template<typename T, typename DstT = T>
class Graph {
public:
    typedef std::make_unsigned<std::ptrdiff_t>::type offset_t;
private:
    bool directed;
    int64_t vertex_number;
    int64_t edge_number;
    offset_t *out_offset;
    DstT *out_neigh;
    offset_t *in_offset;
    DstT *in_neigh;

    struct Neighborhood {
        T n;
        offset_t *offset;
        T *neigh;

        typedef DstT const *iterator;
        iterator begin() { return neigh + offset[n]; }
        iterator end() { return neigh + offset[n + 1]; }
    };
public:
    Graph(): directed{false}, vertex_number{0}, edge_number{0}, out_offset{nullptr},
        out_neigh{nullptr}, in_offset{nullptr}, in_neigh{nullptr} {};
    Graph(int64_t vertex_number, offset_t *&out_offset, DstT *&out_neigh);
    Graph(int64_t vertex_number, offset_t *&out_offset, DstT *&out_neigh,
          offset_t *&in_offset, DstT *&in_neigh);
    Graph(Graph<T, DstT> const &graph) = delete;
    Graph(Graph<T, DstT> &&graph) noexcept;
    ~Graph();

    Graph<T, DstT> &operator=(Graph<T, DstT> const &other) = delete;
    Graph<T, DstT> &operator=(Graph<T, DstT> &&other) noexcept;

    [[nodiscard]] int64_t get_vertex_number() const { return vertex_number; }
    [[nodiscard]] int64_t get_edge_number() const { return edge_number; }
    [[nodiscard]] bool is_directed() const { return directed; }
    offset_t out_degree(T n) const { return out_offset[n + 1] - out_offset[n]; }
    offset_t in_degree(T n) const { return in_offset[n + 1] - in_offset[n]; }
    Neighborhood out_neighbors(T n) const { return {n, out_offset, out_neigh}; }
    Neighborhood in_neighbors(T n) const { return {n, in_offset, in_neigh}; }

    template<typename TT, typename DstTT>
    friend Graph<TT, DstTT> simplify_graph(Graph<TT, DstTT> &raw);
};

template<typename T, typename DstT>
Graph<T, DstT>::Graph(int64_t vertex_number, offset_t *&out_offset, DstT *&out_neigh)
    : directed{false}, vertex_number{vertex_number} {
    this->out_offset = std::exchange(out_offset, nullptr);
    this->out_neigh = std::exchange(out_neigh, nullptr);
    this->in_offset = this->out_offset;
    this->in_neigh = this->out_neigh;
    edge_number = (this->out_offset[vertex_number] - this->out_offset[0]) / 2;
}

template<typename T, typename DstT>
Graph<T, DstT>::Graph(int64_t vertex_number, offset_t *&out_offset, DstT *&out_neigh,
                      offset_t *&in_offset, DstT *&in_neigh)
    : Graph<T, DstT>{vertex_number, out_offset, out_neigh} {
    this->directed = true;
    this->in_offset = std::exchange(in_offset, nullptr);
    this->in_neigh = std::exchange(in_neigh, nullptr);
    edge_number = this->out_offset[vertex_number] - this->out_offset[0];
}

template<typename T, typename DstT>
Graph<T, DstT>::Graph(Graph<T, DstT> &&graph) noexcept
    : directed{std::move(graph.directed)}, vertex_number{std::move(graph.vertex_number)},
    edge_number{std::move(graph.edge_number)} {
    out_offset = std::exchange(graph.out_offset, nullptr);
    out_neigh = std::exchange(graph.out_neigh, nullptr);
    in_offset = std::exchange(graph.in_offset, nullptr);
    in_neigh = std::exchange(graph.in_neigh, nullptr);
}

template<typename T, typename DstT>
Graph<T, DstT>::~Graph() {
    delete[] out_offset;
    delete[] out_neigh;
    if (directed) {
        delete[] in_offset;
        delete[] in_neigh;
    }
}

template<typename T, typename DstT>
Graph<T, DstT> &Graph<T, DstT>::operator=(Graph<T, DstT> &&other) noexcept {
    if (this == &other)
        return *this;
    delete[] this->out_offset;
    delete[] this->out_neigh;
    if (this->directed) {
        delete[] this->in_offset;
        delete[] this->in_neigh;
    }
    this->directed = std::exchange(other.directed, false);
    this->vertex_number = std::exchange(other.vertex_number, 0);
    this->edge_number = std::exchange(other.edge_number, 0);
    this->out_offset = std::exchange(other.out_offset, nullptr);
    this->out_neigh = std::exchange(other.out_neigh, nullptr);
    this->in_offset = std::exchange(other.in_offset, nullptr);
    this->in_neigh = std::exchange(other.in_neigh, nullptr);
    return *this;
}

template<typename T, typename DstT>
std::tuple<Graph<T, DstT>, std::vector<T>, std::vector<T>> reorder_by_degree(Graph<T, DstT> const &g) {
    typedef typename Graph<T, DstT>::offset_t offset_t;
    typedef std::pair<offset_t, T> DegreeNVertex;
    std::vector<DegreeNVertex> degree_vertex_pairs;
    for (int i = 0; i < g.get_vertex_number(); ++i) {
        degree_vertex_pairs.emplace_back(g.out_degree(i), i);
    }
    std::sort(degree_vertex_pairs.begin(), degree_vertex_pairs.end(), std::greater<DegreeNVertex>());
    int64_t vertex_number = g.get_vertex_number();
    std::vector<T> out_degrees(vertex_number, 0);
    std::vector<T> new_ids(vertex_number, 0);
    std::vector<T> new_ids_remap(vertex_number, 0);
    for (int i = 0; i < vertex_number; ++i) {
        out_degrees[i] = degree_vertex_pairs[i].first;
        new_ids[degree_vertex_pairs[i].second] = i;
        new_ids_remap[i] = degree_vertex_pairs[i].second;
    }
    offset_t *out_offset = new offset_t[vertex_number + 1];
    offset_t curr{};
    for (int i = 0; i < vertex_number; ++i) {
        out_offset[i] = curr;
        curr += out_degrees[i];
    }
    out_offset[vertex_number] = curr;
    int64_t edge_number = curr;
    DstT *out_neigh = new DstT[edge_number];
    offset_t *tmp = new offset_t[vertex_number + 1];
    std::copy(out_offset, out_offset + (vertex_number + 1), tmp);
    for (int u = 0; u < vertex_number; ++u) {
        for (DstT const &v : g.out_neighbors(u)) {
            out_neigh[tmp[new_ids[u]]] = new_ids[get_dst_id(v)];
            tmp[new_ids[u]]++;
        }
        std::sort(&out_neigh[out_offset[new_ids[u]]],
                  &out_neigh[out_offset[new_ids[u]+1]],
                  [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) < get_dst_id(rhs); });
    }
    if (!g.is_directed()) {
        delete[] tmp;
        return {Graph<T, DstT>{vertex_number, out_offset, out_neigh}, std::move(new_ids), std::move(new_ids_remap)};
    }
    std::vector<T> in_degrees(vertex_number, 0);
    for (int i = 0; i < vertex_number; ++i) {
        in_degrees[new_ids[i]] = g.in_degree(i);
    }
    offset_t *in_offset = new offset_t[vertex_number + 1];
    curr = 0;
    for (int i = 0; i < vertex_number; ++i) {
        in_offset[i] = curr;
        curr += in_degrees[i];
    }
    in_offset[vertex_number] = curr;
    DstT *in_neigh = new DstT[edge_number];
    std::copy(in_offset, in_offset + (vertex_number + 1), tmp);
    for (int u = 0; u < vertex_number; ++u) {
        for (DstT const &v : g.in_neighbors(u)) {
            in_neigh[tmp[new_ids[u]]] = new_ids[get_dst_id(v)];
            tmp[new_ids[u]]++;
        }
        std::sort(&in_neigh[in_offset[new_ids[u]]],
                  &in_neigh[in_offset[new_ids[u]+1]],
                  [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) < get_dst_id(rhs); });
    }
    delete[] tmp;
    return {Graph<T, DstT>{vertex_number, out_offset, out_neigh, in_offset, in_neigh},
            std::move(new_ids),
            std::move(new_ids_remap)};
}

template<typename T, typename DstT>
std::tuple<Graph<T, DstT>, std::vector<T>, std::vector<T>> squeeze_graph(Graph<T, DstT> const &g) {
    int64_t mapped_src{};
    std::vector<T> vertex_map(g.get_vertex_number(), 0);
    std::vector<T> vertex_remap(g.get_vertex_number(), 0);
    for (int u = 0; u < g.get_vertex_number(); ++u) {
        vertex_map[u] = mapped_src;
        if (g.out_degree(u) != 0 || g.in_degree(u) != 0) {
            vertex_remap[mapped_src] = u;
            mapped_src++;
        }
    }
    typedef typename Graph<T, DstT>::offset_t offset_t;
    int64_t squeezed_vertex_number = mapped_src;
    std::vector<offset_t> out_degrees(squeezed_vertex_number, 0);
    for (T u = 0; u < squeezed_vertex_number; ++u) {
        out_degrees[u] = g.out_degree(vertex_remap[u]);
    }
    offset_t *out_offset = new offset_t[mapped_src + 1];
    offset_t curr{};
    for (T u = 0; u < squeezed_vertex_number; ++u) {
        out_offset[u] = curr;
        curr += out_degrees[u];
    }
    out_offset[squeezed_vertex_number] = curr;
    int64_t edge_number = curr;
    DstT *out_neigh = new DstT[edge_number];
    offset_t *tmp = new offset_t[squeezed_vertex_number + 1];
    std::copy(out_offset, out_offset + (squeezed_vertex_number + 1), tmp);
    for (T u = 0; u < squeezed_vertex_number ; ++u) {
        for (DstT const &v : g.out_neighbors(vertex_remap[u])) {
            out_neigh[tmp[u]] = vertex_map[get_dst_id(v)];
            tmp[u]++;
        }
        std::sort(&out_neigh[out_offset[u]],
                  &out_neigh[out_offset[u+1]],
                  [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) < get_dst_id(rhs); });
    }
    if (!g.is_directed()) {
        delete[] tmp;
        return {
            Graph<T, DstT>{squeezed_vertex_number, out_offset, out_neigh},
            std::move(vertex_map),
            std::move(vertex_remap)
        };
    }
    std::vector<offset_t> in_degrees(std::move(out_degrees));
    for (T u = 0; u < squeezed_vertex_number; ++u) {
        in_degrees[u] = g.in_degree(vertex_remap[u]);
    }
    offset_t *in_offset = new offset_t[squeezed_vertex_number + 1];
    curr = 0;
    for (T u = 0; u < squeezed_vertex_number; ++u) {
        in_offset[u] = curr;
        curr += in_degrees[u];
    }
    in_offset[squeezed_vertex_number] = curr;
    DstT *in_neigh = new DstT[edge_number];
    std::copy(in_offset, in_offset + (squeezed_vertex_number + 1), tmp);
    for (T u = 0; u < squeezed_vertex_number; ++u) {
        for (DstT const &v : g.in_neighbors(vertex_remap[u])) {
            in_neigh[tmp[u]] = vertex_map[get_dst_id(v)];
            tmp[u]++;
        }
        std::sort(&in_neigh[in_offset[u]],
                  &in_neigh[in_offset[u+1]],
                  [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) < get_dst_id(rhs); });
    }
    delete[] tmp;
    return {
            Graph<T, DstT>{squeezed_vertex_number, out_offset, out_neigh, in_offset, in_neigh},
            std::move(vertex_map),
            std::move(vertex_remap)
    };
}

/**
 * WARNING: The function will change the order of edgelist in raw
*/
template<typename T, typename DstT>
Graph<T, DstT> simplify_graph(Graph<T, DstT> &raw) {
    typedef typename Graph<T, DstT>::offset_t offset_t;
    int64_t vertex_number = raw.vertex_number;
    std::vector<offset_t> out_degrees(vertex_number, 0);
    for (T u = 0; u < vertex_number; ++u) {
        out_degrees[u] = std::distance(&raw.out_neigh[raw.out_offset[u]], 
            std::unique(&raw.out_neigh[raw.out_offset[u]], &raw.out_neigh[raw.out_offset[u+1]], 
                [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) == get_dst_id(rhs); }));
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
    for (T u = 0; u < vertex_number; ++u) {
        std::copy(&raw.out_neigh[raw.out_offset[u]], &raw.out_neigh[raw.out_offset[u]+out_degrees[u]], &out_neigh[out_offset[u]]);
    }
    if (!raw.directed) {
        return {vertex_number, out_offset, out_neigh};
    }
    std::vector<offset_t> in_degrees(vertex_number, 0);
    for (T u = 0; u < vertex_number; ++u) {
        in_degrees[u] = std::distance(&raw.in_neigh[raw.in_offset[u]], 
            std::unique(&raw.in_neigh[raw.in_offset[u]], &raw.in_neigh[raw.in_offset[u+1]], 
                [](DstT const &lhs, DstT const &rhs) { return get_dst_id(lhs) == get_dst_id(rhs); }));
    }
    offset_t *in_offset = new offset_t[vertex_number + 1];
    curr = 0;
    for (size_t i = 0; i < vertex_number; ++i) {
        in_offset[i] = curr;
        curr += in_degrees[i];
    }
    in_offset[vertex_number] = curr;
    assert(edge_number == in_offset[vertex_number]);
    DstT *in_neigh = new DstT[edge_number];
    for (T u = 0; u < vertex_number; ++u) {
        std::copy(&raw.in_neigh[raw.in_offset[u]], &raw.in_neigh[raw.in_offset[u]+in_degrees[u]], &in_neigh[in_offset[u]]);
    }
    return {vertex_number, out_offset, out_neigh, in_offset, in_neigh};
}

