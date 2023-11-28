//
// Created by aozora on 11/27/23.
//
#include "utils.h"
#include <numeric>
#include <iostream>
#include <filesystem>

namespace fs = std::filesystem;

constexpr int block_size = 65536;

void get_block_num() {
}
template<typename T, typename... Types>
void get_block_num(T vertex_num, Types ...vertex_nums) {
    std::cout << (vertex_num + block_size - 1) / block_size << " " << vertex_num / block_size << ", ";
    get_block_num(vertex_nums...);
}

int main(int argc, char *argv[]) {
    static long double buf[131071];

    fs::path record_path(RECORD_PATH);
    std::string graph_file{"rmat_17.txt"};

    std::ifstream in;
    in.open((record_path/(graph_file + "-reorder_vertex_edge_visit.bin")).string(), std::ios::in | std::ios::binary);
    in.read(reinterpret_cast<char*>(buf), sizeof(long double) * 131071);
    in.close();

    long double sum = std::reduce(std::begin(buf), std::end(buf), 0.0, std::plus<>());
    std::cout << (sum / 131071 / 131071) << std::endl;

    return 0;
}