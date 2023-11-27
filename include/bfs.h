#ifndef EXPERIMENT_BFS_H
#define EXPERIMENT_BFS_H

#include "graph.h"
#include "memory.h"
#include <vector>
#include <random>
#include <type_traits>

template<typename T>
constexpr T get_max_prop() {
    return T{-1};
}

template<typename T>
constexpr bool is_max_prop(T const &prop) {
    return prop == get_max_prop<T>();
}

template<typename T, typename DstT>
std::vector<T> pick_sources(Graph<T, DstT> const &graph, int n) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<T> dist(0, static_cast<T>(graph.get_vertex_number() - 1));

    std::vector<int> sources; sources.reserve(n);
    for (int i{}; i < n; ++i) {
        T rn;
        do {
            rn = dist(rng);
        } while (graph.out_degree(rn) == 0);
        sources.emplace_back(rn);
    }
    return sources;
}

template<typename T, typename DstT, typename PropT = int>
std::vector<PropT> do_bfs(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    std::queue<T> frontier;
    depth[root] = 0;
    frontier.emplace(root);
    while (!frontier.empty()) {
        T u = frontier.front(); frontier.pop();
        for (auto const &v : graph.out_neighbors(u)) {
            if (is_max_prop(depth[v])) {
                depth[get_dst_id(v)] = depth[u] + 1;
                frontier.emplace(get_dst_id(v));
            }
        }
    }
    return depth;
}


template<typename T, typename DstT, typename AddrT, typename PropT = int>
long long do_cacheline_bfs(Graph<T, DstT> const &graph, T root, Memory<AddrT> &memory) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    depth[root] = 0;
    long long total_edge_vis{};
    int sum = 1;
    int iter{};
    while (sum > 0) {
        sum = 0;
        memory.reset(); // cache expire
        for (T v = 0; v < graph.get_vertex_number(); ++v) {
            if (is_max_prop(depth[v])) {
                bool flag = false;
                for (auto const &u : graph.in_neighbors(v)) {
                    if (!is_max_prop(depth[u]) && depth[u] == iter) {
                        flag = true;
                        break;
                    }
                }
                if (flag) {
                    sum++;
                    int now_edge_offset{};
                    for (auto const &u : graph.in_neighbors(v)) {
                        total_edge_vis += memory.access(v, now_edge_offset);
                        if (!is_max_prop(depth[u]) && depth[u] == iter) {
                            depth[v] = depth[u] + 1;
                            break;
                        }
                        now_edge_offset++;
                    }
                }
            }
        }
        iter++;
    }
    return total_edge_vis;
}

#endif //EXPERIMENT_BFS_H
