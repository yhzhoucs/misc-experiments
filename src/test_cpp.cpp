//
// Created by aozora on 11/27/23.
//
#include "utils.h"
#include <numeric>
#include <iostream>

int main(int argc, char *argv[]) {
    static long double buf[131071];

    std::ifstream in;
    in.open("rmat_17.txt-vertex_edge_visit.bin", std::ios::in | std::ios::binary);
    in.read(reinterpret_cast<char*>(buf), sizeof(long double) * 131071);
    in.close();

    long double sum = std::reduce(std::begin(buf), std::end(buf), 0.0, std::plus<>());
    std::cout << (sum / 131071 / 64) << std::endl;

    return 0;
}