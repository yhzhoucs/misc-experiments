//
// Created by aozora on 11/27/23.
//
#include "utils.h"
#include <numeric>
#include <iostream>
#include <filesystem>
#include <ranges>
#include <execution>
#include <format>

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
    std::string file_name{"/home/aozora/TempData/soc-pokec-relationships.txt-parent_cnt.bin"};

    std::ifstream in(file_name, std::ios::in | std::ios::binary);
    if (!in.is_open()) {
        std::cout << "ERROR OPEN FILE" << std::endl;
        exit(-1);
    }
    in.seekg(0, std::ios::end);
    std::streampos sz = in.tellg();
    in.seekg(0, std::ios::beg);
    size_t vertex_number = sz / sizeof(long double);
    std::vector<long double> freq(vertex_number);
    in.read(reinterpret_cast<char*>(freq.data()), sz);
    in.close();

    auto max_elem = std::ranges::max_element(freq);
    std::cout << std::format("Max: {}", *max_elem) << std::endl;
    std::cout << std::format("Max Unsigned Long Long: {}", std::numeric_limits<unsigned long long>::max()) << std::endl;
    bool valid = std::transform_reduce(std::execution::par, freq.begin(), freq.end(), true,
                        std::logical_and<>(),
                        [](long double const &f) {
                            return f >= 0.0;
                        });
    std::cout << "Valid: " << valid << std::endl;
    return 0;
}