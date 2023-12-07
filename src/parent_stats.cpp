#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include "plf_nanotimer.h"
#include "utils.h"
#include <omp.h>
#include <filesystem>
#include <format>

namespace fs = std::filesystem;

using Node = int;
using Prop = int;

constexpr int block_size = 65536;
constexpr int vertex_offset = 327680;

int main(int argc, char *argv[]) {
    omp_set_num_threads(80); // set to an appropriate number according to the CPU cores

    plf::nanotimer timer;

    fs::path graph_file_path(DATASET_PATH);

    std::vector<std::string> graph_files{
            // "rmat_18.txt",
            // "rmat_19.txt",
            // "rmat_20.txt",
            // "soc-Slashdot0811.txt",
            // "soc-Slashdot0902.txt",
            // "soc-LiveJournal1.txt",
            // "com-orkut.ungraph.txt",
            "soc-pokec-relationships.txt"
    };
    std::vector<std::string> directed{
            "soc-Slashdot0811.txt",
            "soc-Slashdot0902.txt",
            "soc-LiveJournal1.txt",
            "soc-pokec-relationships.txt"
    };

    for (auto const &graph_file: graph_files) {
        std::clog << "Starting to process " << graph_file << std::endl;
        std::clog << "Dataset path: " << (graph_file_path / graph_file).string() << std::endl;
        bool is_directed_graph = (std::find(directed.begin(), directed.end(), graph_file) != directed.end());
        std::clog << "Directed/Need Symmetrize: " << is_directed_graph << std::endl;
        timer.start();
        Builder<Node> builder{(graph_file_path / graph_file).string(), is_directed_graph};
        Graph<Node> graph = builder.build_csr();
        std::clog << "Graph Construction: " << timer.get_elapsed_ms() << " ms" << std::endl;

        // int non_iso_count{};
        // Node vertex_to_go{};
        // for (Node i : std::views::iota(0, graph.get_vertex_number())) {
        //     vertex_to_go++;
        //     if (graph.out_degree(i) > 0) {
        //         non_iso_count++;
        //         if (non_iso_count == graph.get_vertex_number()/200)
        //             break;
        //     }
        // }

        // std::cout << "Vertex To Go: " << vertex_to_go << std::endl;
        // std::cout << "Non Isolated Number: " << non_iso_count << " " << graph.get_vertex_number()/200 << std::endl;

        std::vector<long double> parent_cnt(graph.get_vertex_number());

        int vertex_to_go = graph.get_vertex_number() - vertex_offset;

        std::cout << std::format("Vertex to go: {}", vertex_to_go) << std::endl;
        int block_number = (vertex_to_go + block_size - 1) / block_size;
        std::cout << std::format("Block Number: {}", block_number) << std::endl;
        // backup data when every block finishes
        for (int block_id{}; block_id < block_number; ++block_id) {
            std::fill(parent_cnt.begin(), parent_cnt.end(), 0.0);

            Node vid_beg = vertex_offset + block_id * block_size;
            Node vid_end = std::min(vertex_offset + (block_id + 1) * block_size, vertex_offset + vertex_to_go);
            std::cout << std::format("Start to process block {} ({}-{})", block_id, vid_beg, vid_end - 1) << std::endl;

            timer.start();
            #pragma omp parallel default(none) shared(graph, vid_beg, vid_end, parent_cnt)
            {
                std::vector<long double> l_parent_cnt(graph.get_vertex_number());
                // #pragma omp for schedule(dynamic, 1024) nowait
                #pragma omp for nowait
                for (Node vid = vid_beg; vid < vid_end; ++vid) {
                    if (graph.out_degree(vid) > 0) {
                        std::vector<Prop> depth = do_bfs(graph, vid);
                        for (Node v = 0; v < graph.get_vertex_number(); ++v) {
                            if (!is_max_prop(depth[v])) {
                                for (auto const &u: graph.in_neighbors(v)) {
                                    if (depth[u] == depth[v] - 1) {
                                        // depth[u] does not equal to -1
                                        // because u is connected to a vertex whose depth is not -1,
                                        // and it's in a symmetric graph
                                        l_parent_cnt[u]++;
                                    }
                                }
                            }
                        }
                    }
                }
#pragma omp critical
                {
                    std::transform(parent_cnt.begin(), parent_cnt.end(),
                                   l_parent_cnt.begin(), parent_cnt.begin(), std::plus<>());
                }
            }
            std::cout << "Block " << block_id << " Finish!" << std::endl;
            std::cout << "Time Elapsed: " << timer.get_elapsed_ms() / 1000 << " s" << std::endl;

            fs::path record_path(RECORD_PATH);
            reduce_backup(parent_cnt, record_path / (graph_file + "-parent_cnt.bin"), std::plus<>());
            std::cout << "Backup Finish!" << std::endl;
        }

        fs::path record_path(RECORD_PATH);
        auto get_degree = [&graph](int vid) { return graph.out_degree(vid); };
        // convert binary record to txt
        convert_to_txt<long double, decltype(get_degree)>(
                record_path / (graph_file + "-parent_cnt.bin"),
                record_path / (graph_file + "-parent_cnt.txt"),
                static_cast<int>(graph.get_vertex_number()),
                get_degree
        );
    }
    return 0;
}
