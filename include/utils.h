//
// Created by aozora on 11/27/23.
//

#ifndef EXPERIMENT_UTILS_H
#define EXPERIMENT_UTILS_H

#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>
#include <ranges>

namespace fs = std::filesystem;

template<typename T, typename BinaryOperation>
void reduce_backup(std::vector<T> &backup, std::string const &file_name, BinaryOperation binary_op) {
    if (!fs::exists(file_name)) {
        std::ofstream out(file_name, std::ios::out | std::ios::binary);
        out.write(reinterpret_cast<char*>(backup.data()), sizeof(T) * backup.size());
        out.close();
    } else {
        std::vector<T> tmp(backup.size());
        std::fstream file(file_name, std::ios::in | std::ios::out | std::ios::binary);
        file.read(reinterpret_cast<char*>(tmp.data()), sizeof(T) * tmp.size());
        file.clear();
        file.seekg(0, std::ios::beg);
        std::transform(tmp.begin(), tmp.end(), backup.begin(), tmp.begin(), binary_op);
        file.write(reinterpret_cast<char*>(tmp.data()), sizeof(T) * backup.size());
        file.close();
    }
}

template<typename T, typename FuncT>
void convert_to_txt(std::string const &binary_file, std::string const &output_file, int vertex_num, FuncT get_degree) {
    std::vector<T> tmp(vertex_num);
    std::fstream in(binary_file, std::ios::in | std::ios::binary);
    in.read(reinterpret_cast<char*>(tmp.data()), sizeof(T) * tmp.size());
    in.close();
    std::fstream out(output_file, std::ios::out | std::ios::trunc);
    for (int i : std::views::iota(0, vertex_num)) {
        out << get_degree(i) << " " << tmp[i] << std::endl;
    }
    out.close();
}


#endif //EXPERIMENT_UTILS_H
