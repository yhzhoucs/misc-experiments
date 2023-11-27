//
// Created by aozora on 11/27/23.
//
#include "utils.h"
#include <numeric>
#include <iostream>

int main(int argc, char *argv[]) {
    static long double buf[1048576];

    std::ifstream in;
    in.open("rmat_20.txtreorder_vertex_edge_visit_cacheline.bin", std::ios::in | std::ios::binary);
    in.read(reinterpret_cast<char*>(buf), sizeof(long double) * 1048576);
    in.close();

    long double sum = std::reduce(std::begin(buf), std::end(buf), 0.0, std::plus<>());
    std::cout << (sum / 1048576 / 64) << std::endl;

    return 0;
}