#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include "bitmap.h"
#include <filesystem>
#include <format>

namespace fs = std::filesystem;

using Node = int;
using Prop = int;

template<typename T, typename U, typename V>
std::ostream &print_info(std::ostream &out, T const &iter, U const &active_vertex, V const &active_edge) {
    out << std::format("{} {:<10d} {:<15d}", iter, active_vertex, active_edge) << std::endl;
    return out;
}

template<typename T, typename DstT, typename PropT = int>
std::vector<PropT> push_active_num_ana(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    std::queue<T> frontier;
    depth[root] = 0;
    frontier.emplace(root);
    int iter{};
    print_info(std::cout, iter, frontier.size(), graph.out_degree(root));
    while (!frontier.empty()) {
        int frontier_size = frontier.size();
        long long active_num{};
        long long total_degree{};
        std::vector<int> frontier_vec;
        frontier_vec.reserve(frontier_size);
        while (!frontier.empty()) {
            frontier_vec.emplace_back(frontier.front());
            frontier.pop();
        }
        // dry run
        for (auto const &u : frontier_vec) {
            for (auto const &v : graph.out_neighbors(u)) {
                if (is_max_prop(depth[v])) {
                    active_num += 1;
                    total_degree += graph.out_degree(v);
                }
            }
        }
        print_info(std::cout, iter + 1, active_num, total_degree);
        for (auto const &u : frontier_vec) {
            for (auto const &v : graph.out_neighbors(u)) {
                if (is_max_prop(depth[v])) {
                    depth[get_dst_id(v)] = depth[u] + 1;
                    frontier.emplace(get_dst_id(v));
                }
            }
        }
        iter++;
    }
    return depth;
}

template<typename T, typename DstT, typename PropT = int>
std::vector<PropT> push_active_num_no_repeat_ana(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    std::queue<T> frontier;
    depth[root] = 0;
    frontier.emplace(root);
    int iter{};
    print_info(std::cout, iter, frontier.size(), graph.out_degree(root));
    while (!frontier.empty()) {
        int frontier_size = frontier.size();
        long long active_num{};
        long long total_degree{};
        while (frontier_size-- > 0) {
            T u = frontier.front(); frontier.pop();
            for (auto const &v : graph.out_neighbors(u)) {
                if (is_max_prop(depth[v])) {
                    active_num += 1;
                    total_degree += graph.out_degree(v);
                    depth[get_dst_id(v)] = depth[u] + 1;
                    frontier.emplace(get_dst_id(v));
                }
            }
        }
        print_info(std::cout, iter + 1, active_num, total_degree);
        iter++;
    }
    return depth;
}

template<typename T, typename DstT, typename PropT>
auto pull_active_helper(Graph<T, DstT> const &graph, std::vector<PropT> const &depth)
    -> std::tuple<long long, long long> {
    long long active_num{};
    long long total_degree{};
    #pragma omp parallel for default(none) shared(graph, depth) reduction(+ : active_num, total_degree)
    for (T u = 0; u < depth.size(); ++u) {
        if (is_max_prop(depth[u])) {
            active_num += 1;
            total_degree += graph.out_degree(u);
        }
    }
    return {active_num, total_degree};
}

template<typename T, typename DstT, typename PropT = int>
std::vector<PropT> pull_active_num_ana(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    depth[root] = 0;
    Bitmap front(graph.get_vertex_number());
    Bitmap next(graph.get_vertex_number());
    front.reset();
    next.reset();
    front.set_bit(root);
    depth[root] = 0;
    int sum = 1;
    int iter{};
    while (sum > 0) {
        sum = 0;
//        auto [active_num, total_degree] = pull_active_helper(graph, depth);
//        print_info(std::cout, iter + 1, active_num, total_degree);
        long long active_v{};
        long long edge_visit{};
        for (T v : std::views::iota(0, graph.get_vertex_number())) {
            if (is_max_prop(depth[v])) {
                active_v++;
                for (auto const &u : graph.in_neighbors(v)) {
                    edge_visit++;
                    if (front.get_bit(u)) {
                        depth[v] = depth[u] + 1;
                        sum++;
                        next.set_bit(v);
                    }
                }
            }
        }
        print_info(std::cout, iter, active_v, edge_visit);
        iter++;
        front.swap(next);
    }
    return depth;
}

template<typename T, typename DstT, typename PropT = int>
std::vector<PropT> pull_eb_active_num_ana(Graph<T, DstT> const &graph, T root) {
    std::vector<PropT> depth(graph.get_vertex_number(), get_max_prop<PropT>());
    depth[root] = 0;
    Bitmap front(graph.get_vertex_number());
    Bitmap next(graph.get_vertex_number());
    front.reset();
    next.reset();
    front.set_bit(root);
    depth[root] = 0;
    int sum = 1;
    int iter{};
    while (sum > 0) {
        sum = 0;
//        auto [active_num, total_degree] = pull_active_helper(graph, depth);
//        print_info(std::cout, iter + 1, active_num, total_degree);
        long long active_v{};
        long long edge_visit{};
        for (T v : std::views::iota(0, graph.get_vertex_number())) {
            if (is_max_prop(depth[v])) {
                active_v++;
                for (auto const &u : graph.in_neighbors(v)) {
                    edge_visit++;
                    if (front.get_bit(u)) {
                        depth[v] = depth[u] + 1;
                        sum++;
                        next.set_bit(v);
                        break;
                    }
                }
            }
        }
        print_info(std::cout, iter, active_v, edge_visit);
        iter++;
        front.swap(next);
    }
    return depth;
}

//int main(int argc, char *argv[]) {
//    fs::path graph_file_path(DATASET_PATH);
//    if (argc < 2) {
//        graph_file_path /= "rmat_20.txt";
//    } else {
//        graph_file_path /= argv[1];
//    }
//
//    Builder<Node> builder{graph_file_path.string(), true};
//    Graph<Node> graph = builder.build_csr();
//    std::clog << "Graph: " << graph_file_path.string() << std::endl;
//
//    Graph<Node> simplified = simplify_graph(graph);
//
//    std::ofstream out("undirected-soc-Livejournal1.txt", std::ios::out | std::ios::trunc);
//    for (Node u : std::views::iota(0, simplified.get_vertex_number())) {
//        for (auto const &v : simplified.out_neighbors(u)) {
//            out << u << " " << v << "\n";
//        }
//        if (u % 1000 == 0) {
//            out.flush();
//        }
//    }
//    out.flush();
//    out.close();
//
//    return 0;
//}

int main(int argc, char *argv[]) {
    fs::path graph_file_path(DATASET_PATH);
    if (argc < 2) {
        graph_file_path /= "rmat_20.txt";
    } else {
        graph_file_path /= argv[1];
    }

    Builder<Node> builder{graph_file_path.string()};
    Graph<Node> graph = builder.build_csr();
    std::clog << "Graph: " << graph_file_path.string() << std::endl;

    std::vector<Node> sources = pick_sources(graph, 1);
    Node root = sources[0];
    std::clog << "Root: " << root << std::endl;

    std::cout << "Push重复" << std::endl;
    std::vector<int> depth_1 = push_active_num_ana(graph, root);
    std::cout << "\nPush不重复" << std::endl;
    std::vector<int> depth_2 = push_active_num_no_repeat_ana(graph, root);
    std::cout << "\nPull" << std::endl;
    std::vector<int> depth_3 = pull_active_num_ana(graph, root);
    std::cout << "\nPull Early Break" << std::endl;
    std::vector<int> depth_4 = pull_eb_active_num_ana(graph, root);
    std::cout << std::endl;

    bool pass = true;
    std::ranges::for_each(std::views::iota(0, graph.get_vertex_number()), [&](Node i) {
        pass = pass && (depth_1[i] == depth_2[i])
                    && (depth_1[i] == depth_3[i])
                    && (depth_1[i] == depth_4[i]);
    });

    if (pass) {
        std::cout << "Verification: PASS" << std::endl;
    } else {
        std::cout << "Verification: FAIL" << std::endl;
    }

    return 0;
}
