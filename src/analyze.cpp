#include <iostream>
#include <fstream>
#include <format>
#include <string>
#include <filesystem>
#include <array>
#include <numeric>

namespace fs = std::filesystem;

constexpr auto tasks = std::array{
    "vertex_edge_visit",
    "reorder_vertex_edge_visit",
    "vertex_edge_visit_cacheline",
    "reorder_vertex_edge_visit_cacheline"
};

template<typename StrT, typename NumT,
    typename=std::enable_if_t<std::is_integral_v<NumT>>>
void analyze(StrT &&graph_name, NumT vertex_num) {
    fs::path path(RECORD_PATH);
    long double *values = new long double[vertex_num];
    std::ifstream in;
    for (const auto &task : tasks) {
        in.open(path / (std::format("{}-{}.bin", graph_name, task)), std::ios::in | std::ios::binary);
        in.read(reinterpret_cast<char*>(values), sizeof(long double) * vertex_num);
        in.close();
        long double sum = std::reduce(values, values + vertex_num, 0.0, std::plus<>());
        long double avg = sum / (static_cast<long double>(vertex_num) * vertex_num);
        std::cout << std::format("{} : avg-{} is {:.2f}", graph_name, task, avg) << std::endl;
    }
    delete[] values;
}

template<typename StrT, typename NumT, typename... Types>
void analyze(StrT &&graph_name, NumT vertex_num, Types... args) {
    analyze(graph_name, vertex_num);
    analyze(args...);
}

int main(int argc, char *argv[]) {
    analyze(
            "rmat_17.txt", 131071,
            "rmat_18.txt", 262143,
            "rmat_19.txt", 524286,
            "rmat_20.txt", 1048576
    );
    return 0;
}