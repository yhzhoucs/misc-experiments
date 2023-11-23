#ifndef EXPERIMENT_BFS_H
#define EXPERIMENT_BFS_H

#include "graph.h"
#include <vector>
#include <random>

template<typename T,
    typename=std::enable_if_t<std::is_signed_v<T>>>
constexpr T get_unvisited_prop() {
    return T{-1};
}

template<typename T>
constexpr bool is_unvisited_prop(T const &prop) {
    return prop == get_unvisited_prop<T>();
}

template<typename T, typename DstT, typename PropT = int,
    typename=std::enable_if_t<std::is_signed_v<PropT>>>
std::vector<PropT> do_bfs(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_unvisited_prop<T>());
    std::queue<T> frontier;
    depth[root] = 0;
    frontier.emplace(root);
    while (!frontier.empty()) {
        T u = frontier.front(); frontier.pop();
        for (auto const &v : graph.out_neighbors(u)) {
            if (is_unvisited_prop(depth[v])) {
                depth[get_dst_id(v)] = depth[u] + 1;
                frontier.emplace(get_dst_id(v));
            }
        }
    }
    return depth;
}

template<typename T, typename DstT>
std::vector<T> pick_sources(Graph<T, DstT> const &graph, int n) {
    std::random_device rd;
    std::mt19937 rng(rd());
    std::uniform_int_distribution<T> dist(0, static_cast<T>(graph.get_vertex_number() - 1));

    std::vector<int> sources; sources.reserve(n);
    for (int i{}; i < n; ++i) {
        sources.emplace_back(dist(rng));
    }
    return sources;
}

#endif //EXPERIMENT_BFS_H
