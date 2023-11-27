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