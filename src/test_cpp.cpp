//
// Created by aozora on 11/24/23.
//
#include <type_traits>
#include <iostream>
#include <ranges>
#include <numeric>
#include <vector>

int main(int argc, char *argv[]) {
    std::vector<int> vec(100);
    std::iota(vec.begin(), vec.end(), 0);

    std::partial_sum(vec.begin(), vec.end(), vec.begin(), std::plus<>());

    std::copy(vec.begin(), vec.end(), std::ostream_iterator<int>(std::cout, ", "));
    std::cout << std::endl;

    return 0;
}