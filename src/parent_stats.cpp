#include "graph.h"
#include "builder.h"
#include "bfs.h"
#include <omp.h>
#include <filesystem>

namespace fs = std::filesystem;

using Node = int;
using Prop = int;

int main(int argc, char *argv[]) {
    omp_set_num_threads(4); // set to an appropriate number according to the CPU cores

    fs::path graph_file_path(GRAPH_FILE_PREFIX);
    if (argc < 2) {
        graph_file_path /= "rmat_17.txt";
    } else {
        graph_file_path /= argv[2];
    }

    Builder<Node> builder{graph_file_path.string()};
    Graph<Node> graph = builder.build_csr();
    std::vector<Node> sources = pick_sources(graph, 64);

    std::vector<long long> parent_cnt(graph.get_vertex_number(), 0);

#pragma omp parallel default(none) shared(graph, sources, parent_cnt)
    {
        std::vector<long long> local_parent_cnt(graph.get_vertex_number(), 0);
#pragma omp for
        for (size_t i = 0; i < sources.size(); ++i) {
            std::vector<Prop> depth = do_bfs(graph, sources[i]);
            for (Node v = 0; v < graph.get_vertex_number(); ++v) {
                if (!is_unvisited_prop(depth[v])) {
                    for (auto const &u: graph.in_neighbors(v)) {
                        if (depth[u] == depth[v] - 1) {
                            // depth[u] does not equal to -1
                            // because u is connected to a vertex whose depth is not -1
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

    return 0;
}