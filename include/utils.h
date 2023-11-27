//
// Created by aozora on 11/27/23.
//

#ifndef EXPERIMENT_UTILS_H
#define EXPERIMENT_UTILS_H

#include <fstream>
#include <vector>
#include <filesystem>
#include <algorithm>

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

#endif //EXPERIMENT_UTILS_H
