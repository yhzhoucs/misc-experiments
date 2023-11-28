#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include "plf_nanotimer.h"
#include <omp.h>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace fs = std::filesystem;

using Node = int;
using Prop = int;

constexpr int block_size = 65536;

int main(int argc, char *argv[]) {
    omp_set_num_threads(4); // set to an appropriate number according to the CPU cores

    plf::nanotimer timer;

    fs::path graph_file_path(DATASET_PATH);

    std::vector<std::string> graph_files{
       "rmat_17.txt",
       "rmat_18.txt",
       "rmat_19.txt",
       "rmat_20.txt"
    };

    for (auto const &graph_file : graph_files) {
        std::clog << "Starting to process " << graph_file << std::endl;
        std::clog << "Dataset path: " << (graph_file_path/graph_file).string() << std::endl;
        timer.start();
        Builder<Node> builder{(graph_file_path/graph_file).string()};
        Graph<Node> graph = builder.build_csr();
        std::clog << "Graph Construction: " << timer.get_elapsed_ms() << " ms" << std::endl;
        timer.start();
        auto [reordered, new_ids, new_ids_remap] = reorder_by_degree(graph);
        std::clog << "Graph Reorder: " << timer.get_elapsed_ms() << " ms" << std::endl;

        std::vector<long double> parent_cnt(graph.get_vertex_number());

        int block_number = (graph.get_vertex_number() + block_size - 1) / block_size;
        // backup data when every block finished
        for (int block_id{}; block_id < block_number; ++block_id) {
            std::fill(parent_cnt.begin(), parent_cnt.end(), 0.0);

            Node vid_beg = block_id * block_size;
            Node vid_end = std::min((block_id + 1) * block_size, static_cast<Node>(graph.get_vertex_number()));
            std::cout << std::format("Start to process block {} ({}-{})", block_id, vid_beg, vid_end - 1) << std::endl;
            timer.start();
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
            std::cout << "Time Elapsed: " << timer.get_elapsed_ms()/1000 << " s" << std::endl;

        }

    timer.start();
    Builder<Node> builder{graph_file_path.string()};
    Graph<Node> graph = builder.build_csr();
    std::clog << "Graph: " << graph_file_path.string() << std::endl;
    std::clog << "Graph Construction: " << timer.get_elapsed_ms() << " ms" << std::endl;

    timer.start();
    std::vector<Node> sources = pick_sources(graph, 64);
    std::clog << "Source Pick: " << timer.get_elapsed_ms() << " ms" << std::endl;

//    std::vector<Node> sources{76294, 39534, 82507, 119159, 63081, 107786, 66976, 49259, 84806, 14105, 115310, 124530, 23212, 30726, 83086, 72855};

    timer.start();
    std::vector<long long> parent_cnt(graph.get_vertex_number(), 0);

#pragma omp parallel default(none) shared(graph, sources, parent_cnt)
    {
        std::vector<long long> local_parent_cnt(graph.get_vertex_number(), 0);
#pragma omp for nowait
        for (size_t i = 0; i < sources.size(); ++i) {
            std::vector<Prop> depth = do_bfs(graph, sources[i]);
            for (Node v = 0; v < graph.get_vertex_number(); ++v) {
                if (!is_max_prop(depth[v])) {
                    for (auto const &u: graph.in_neighbors(v)) {
                        if (depth[u] == depth[v] - 1) {
                            // depth[u] does not equal to -1
                            // because u is connected to a vertex whose depth is not -1,
                            // and it's in a symmetric graph
                            local_parent_cnt[u]++;
                        }
                    }
                }
            }
        }
#pragma omp critical
        {
            std::transform(parent_cnt.begin(), parent_cnt.end(), local_parent_cnt.begin(), parent_cnt.begin(), std::plus<>());
        }
    }
    std::clog << "Processing: " << timer.get_elapsed_ms() << " ms" << std::endl;

    std::ofstream out(graph_file_path.filename().string() + "-parent_stats.txt", std::ios::out | std::ios::trunc);
    std::copy(parent_cnt.begin(), parent_cnt.end(), std::ostream_iterator<int>(out, "\n"));
    out.flush();
    out.close();

    return 0;
}