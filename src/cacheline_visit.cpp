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

    fs::path graph_file_path(DATASET_PATH);
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
    std::vector<Node> sources = pick_sources(graph, 16);
    std::clog << "Source Pick: " << timer.get_elapsed_ms() << " ms" << std::endl;

    int non_iso_num{};
    #pragma omp parallel for reduction(+ : non_iso_num)
    for (Node i = 0; i < graph.get_vertex_number(); ++i) {
        if (graph.out_degree(i) > 0) {
            non_iso_num++;
        }
    }
    std::cout << "Non Iso Num: " << non_iso_num << std::endl;

    timer.start();
    long double edge_visit{};
    long double edge_visit_cacheline{};
    #pragma omp parallel default(none) shared(graph, sources, edge_visit, edge_visit_cacheline)
    {
        long double l_edge_visit{};
        long double l_edge_visit_cacheline{};
        Memory<unsigned> memory{graph};
        #pragma omp for nowait
        for (size_t i = 0; i < sources.size(); ++i) {
            auto [t_visit, t_visit_cacheline] = do_cacheline_bfs(graph, sources[i], memory);
            l_edge_visit += t_visit;
            l_edge_visit_cacheline += t_visit_cacheline;
        }
        #pragma omp atomic
        edge_visit = edge_visit + l_edge_visit;
        #pragma omp atomic
        edge_visit_cacheline = edge_visit_cacheline + l_edge_visit_cacheline;
    }
    std::clog << "Processing: " << timer.get_elapsed_ms() << " ms" << std::endl;

    timer.start();
    long double reorder_edge_visit{};
    long double reorder_edge_visit_cacheline{};
    #pragma omp parallel default(none) shared(reordered, new_ids, sources, reorder_edge_visit, reorder_edge_visit_cacheline)
    {
        long double l_edge_visit{};
        long double l_edge_visit_cacheline{};
        Memory<unsigned> memory{reordered};
        #pragma omp for nowait
        for (size_t i = 0; i < sources.size(); ++i) {
            auto [t_visit, t_visit_cacheline] = do_cacheline_bfs(reordered, new_ids[sources[i]], memory);
            l_edge_visit += t_visit;
            l_edge_visit_cacheline += t_visit_cacheline;
        }
        #pragma omp atomic
        reorder_edge_visit = reorder_edge_visit + l_edge_visit;
        #pragma omp atomic
        reorder_edge_visit_cacheline = reorder_edge_visit_cacheline + l_edge_visit_cacheline;
    }
    std::clog << "Processing Reordered: " << timer.get_elapsed_ms() << " ms" << std::endl;

    std::cout << std::format("Vertex-Avg Edge Visit: {:.2f}", edge_visit / sources.size() / graph.get_vertex_number()) << std::endl;
    std::cout << std::format("Non-Iso-Vertex-Avg Edge Visit: {:.2f}", edge_visit / sources.size() / non_iso_num) << std::endl;
    std::cout << std::format("Vertex-Avg Edge Visit Cacheline: {:.2f}", edge_visit_cacheline / sources.size() / graph.get_vertex_number()) << std::endl;
    std::cout << std::format("Non-Iso-Vertex-Avg Edge Visit Cacheline: {:.2f}", edge_visit_cacheline / sources.size() / non_iso_num) << std::endl;

    std::cout << std::format("Reorder Vertex-Avg Edge Visit: {:.2f}", reorder_edge_visit / sources.size() / graph.get_vertex_number()) << std::endl;
    std::cout << std::format("Reorder Non-Iso-Vertex-Avg Edge Visit: {:.2f}", reorder_edge_visit / sources.size() / non_iso_num) << std::endl;
    std::cout << std::format("Reorder Vertex-Avg Edge Visit Cacheline: {:.2f}", reorder_edge_visit_cacheline / sources.size() / graph.get_vertex_number()) << std::endl;
    std::cout << std::format("Reorder Non-Iso-Vertex-Avg Edge Visit Cacheline: {:.2f}", reorder_edge_visit_cacheline / sources.size() / non_iso_num) << std::endl;
}