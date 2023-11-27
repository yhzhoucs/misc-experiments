#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include "plf_nanotimer.h"
#include <omp.h>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <format>

namespace fs = std::filesystem;

using Node = int;
using Prop = int;

int main(int argc, char *argv[]) {
    omp_set_num_threads(4); // set to an appropriate number according to the CPU cores

    plf::nanotimer timer;

    fs::path graph_file_path(GRAPH_FILE_PREFIX);
    if (argc < 2) {
        graph_file_path /= "rmat_17.txt";
    } else {
        graph_file_path /= argv[1];
    }

    timer.start();
    Builder<Node> builder{graph_file_path.string()};
    Graph<Node> graph = builder.build_csr();
    std::clog << "Graph: " << graph_file_path.string() << std::endl;
    std::clog << "Graph Construction: " << timer.get_elapsed_ms() << " ms" << std::endl;
    timer.start();
    auto [reordered, new_ids, new_ids_remap] = reorder_by_degree(graph);
    std::clog << "Graph Reorder: " << timer.get_elapsed_ms() << " ms" << std::endl;

    timer.start();
    std::vector<Node> sources = pick_sources(graph, 64);
//    std::vector<Node> sources{76294, 39534, 82507, 119159, 63081, 107786, 66976, 49259, 84806, 14105, 115310, 124530, 23212, 30726, 83086, 72855};
    std::clog << "Source Pick: " << timer.get_elapsed_ms() << " ms" << std::endl;

    timer.start();
    long double edge_vis{};
#pragma omp parallel default(none) shared(graph, sources, edge_vis)
    {
        long double local_edge_vis{};
        Memory<unsigned> memory{graph};
#pragma omp for nowait
        for (size_t i = 0; i < sources.size(); ++i) {
            local_edge_vis += do_cacheline_bfs(graph, sources[i], memory);
        }
#pragma omp atomic
        edge_vis = edge_vis + local_edge_vis;
    }
    std::clog << "Processing: " << timer.get_elapsed_ms() << " ms" << std::endl;

    timer.start();
    long double reordered_edge_vis{};
#pragma omp parallel default(none) shared(reordered, new_ids, sources, reordered_edge_vis)
    {
        long double local_edge_vis{};
        Memory<unsigned> memory{reordered};
#pragma omp for nowait
        for (size_t i = 0; i < sources.size(); ++i) {
            local_edge_vis += do_cacheline_bfs(reordered, new_ids[sources[i]], memory);
        }
#pragma omp atomic
        reordered_edge_vis = reordered_edge_vis + local_edge_vis;
    }
    std::clog << "Processing Reordered: " << timer.get_elapsed_ms() << " ms" << std::endl;

    std::cout << std::format("Total Edge Visit: {}", edge_vis) << std::endl;
    std::cout << std::format("Avg Edge Visit: {}", edge_vis / sources.size()) << std::endl;
    std::cout << std::format("Vertex-Avg Edge Visit: {}", edge_vis / sources.size() / graph.get_vertex_number()) << std::endl;

    std::cout << std::format("Total Edge Visit Reordered: {}", reordered_edge_vis) << std::endl;
    std::cout << std::format("Avg Edge Visit Reordered: {}", reordered_edge_vis / sources.size()) << std::endl;
    std::cout << std::format("Vertex-Avg Edge Visit Reordered: {}", reordered_edge_vis / sources.size() / graph.get_vertex_number()) << std::endl;
}