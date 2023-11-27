#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include "utils.h"
#include <cstring>
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

    std::vector<std::string> graph_files{
        "rmat_17.txt",
        "rmat_18.txt",
        "rmat_19.txt",
        "rmat_20.txt"
    };

    for (auto const &graph_file: graph_files) {
        std::clog << "Starting to process " << graph_file << std::endl;
        std::clog << "Dataset path: " << (graph_file_path/graph_file).string() << std::endl;
        timer.start();
        Builder<Node> builder{(graph_file_path/graph_file).string()};
        Graph<Node> graph = builder.build_csr();
        std::clog << "Graph Construction: " << timer.get_elapsed_ms() << " ms" << std::endl;
        timer.start();
        auto [reordered, new_ids, new_ids_remap] = reorder_by_degree(graph);
        std::clog << "Graph Reorder: " << timer.get_elapsed_ms() << " ms" << std::endl;

        std::cout << graph.get_vertex_number() << std::endl;

        timer.start();
        int block_size = 65536;

        std::vector<long double> vertex_edge_visit(graph.get_vertex_number());
        std::vector<long double> vertex_edge_visit_cacheline(graph.get_vertex_number());
        std::vector<long double> reorder_vertex_edge_visit(graph.get_vertex_number());
        std::vector<long double> reorder_vertex_edge_visit_cacheline(graph.get_vertex_number());

        // backup data when every block finished
        for (int block_id{}; block_id < graph.get_vertex_number() / block_size; ++block_id) {
            std::fill(vertex_edge_visit.begin(), vertex_edge_visit.end(), 0.0);
            std::fill(vertex_edge_visit_cacheline.begin(), vertex_edge_visit_cacheline.end(), 0.0);
            std::fill(reorder_vertex_edge_visit.begin(), reorder_vertex_edge_visit.end(), 0.0);
            std::fill(reorder_vertex_edge_visit_cacheline.begin(), reorder_vertex_edge_visit_cacheline.end(), 0.0);

            Node vid_beg = block_id * block_size;
            Node vid_end = std::min((block_id + 1) * block_size, static_cast<Node>(graph.get_vertex_number()));
            std::cout << std::format("Start to process block {} ({}-{})", block_id, vid_beg, vid_end - 1) << std::endl;
            #pragma omp parallel default(none) shared(graph, vid_beg, vid_end, vertex_edge_visit, vertex_edge_visit_cacheline)
            {
                std::vector<long double> l_vertex_edge_visit(graph.get_vertex_number());
                std::vector<long double> l_vertex_edge_visit_cacheline(graph.get_vertex_number());
                Memory<unsigned> memory{graph};
                #pragma omp for schedule(dynamic, 1024) nowait
                for (Node vid = vid_beg; vid < vid_end; ++vid) {
                    if (graph.out_degree(vid) > 0) {
                        auto [t_visit, t_visit_cacheline] = do_cacheline_bfs(graph, vid, memory);
                        std::transform(l_vertex_edge_visit.begin(), l_vertex_edge_visit.end(),
                                       t_visit.begin(), l_vertex_edge_visit.begin(), std::plus<>());
                        std::transform(l_vertex_edge_visit_cacheline.begin(), l_vertex_edge_visit_cacheline.end(),
                                       t_visit_cacheline.begin(), l_vertex_edge_visit_cacheline.begin(), std::plus<>());
                    }
                }
                #pragma omp critical
                {
                    std::transform(vertex_edge_visit.begin(), vertex_edge_visit.end(),
                                   l_vertex_edge_visit.begin(), vertex_edge_visit.begin(), std::plus<>());
                    std::transform(vertex_edge_visit_cacheline.begin(), vertex_edge_visit_cacheline.end(),
                                   l_vertex_edge_visit_cacheline.begin(), vertex_edge_visit_cacheline.begin(), std::plus<>());
                }
            }

            #pragma omp parallel default(none) shared(reordered, new_ids, vid_beg, vid_end, reorder_vertex_edge_visit, reorder_vertex_edge_visit_cacheline)
            {
                std::vector<long double> l_vertex_edge_visit(reordered.get_vertex_number());
                std::vector<long double> l_vertex_edge_visit_cacheline(reordered.get_vertex_number());
                Memory<unsigned> memory{reordered};
                #pragma omp for schedule(dynamic, 1024) nowait
                for (Node vid = vid_beg; vid < vid_end; ++vid) {
                    if (reordered.out_degree(new_ids[vid]) > 0) {
                        auto [t_visit, t_visit_cacheline] = do_cacheline_bfs(reordered, new_ids[vid], memory);
                        std::transform(l_vertex_edge_visit.begin(), l_vertex_edge_visit.end(),
                                       t_visit.begin(), l_vertex_edge_visit.begin(), std::plus<>());
                        std::transform(l_vertex_edge_visit_cacheline.begin(), l_vertex_edge_visit_cacheline.end(),
                                       t_visit_cacheline.begin(), l_vertex_edge_visit_cacheline.begin(), std::plus<>());
                    }
                }
                #pragma omp critical
                {
                    std::transform(reorder_vertex_edge_visit.begin(), reorder_vertex_edge_visit.end(),
                                   l_vertex_edge_visit.begin(), reorder_vertex_edge_visit.begin(), std::plus<>());
                    std::transform(reorder_vertex_edge_visit_cacheline.begin(), reorder_vertex_edge_visit_cacheline.end(),
                                   l_vertex_edge_visit_cacheline.begin(), reorder_vertex_edge_visit_cacheline.begin(), std::plus<>());
                }
            }

            std::cout << "Block " << block_id << " Finish!" << std::endl;
            reduce_backup(vertex_edge_visit, graph_file + "-vertex_edge_visit.bin", std::plus<>());
            reduce_backup(vertex_edge_visit_cacheline, graph_file + "-vertex_edge_visit_cacheline.bin", std::plus<>());
            reduce_backup(reorder_vertex_edge_visit, graph_file + "-reorder_vertex_edge_visit.bin", std::plus<>());
            reduce_backup(reorder_vertex_edge_visit_cacheline, graph_file + "-reorder_vertex_edge_visit_cacheline.bin", std::plus<>());
            std::cout << "Backup Finish!" << std::endl;
        }
    }
    return 0;
}