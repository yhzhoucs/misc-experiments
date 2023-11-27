#ifndef EXPERIMENT_MEMORY_H
#define EXPERIMENT_MEMORY_H

#include "bitmap.h"
#include "graph.h"
#include <vector>
#include <ranges>
#include <numeric>

constexpr int cacheline_edge_num = 16;

template<typename T>
inline
constexpr T get_aligned_size(T sz, T align = cacheline_edge_num) {
    if (sz == 0) {
        return align;
    } else {
        return (((sz - 1) / align) + 1) * align;
    }
}

template<typename SizeT, typename GraphT>
inline
constexpr SizeT cal_mem_size(GraphT const &graph) {
    return (get_aligned_size(graph.get_vertex_number())
                + get_aligned_size(graph.get_edge_number()));
}

template<typename T>
class Memory {
private:
    Bitmap bmp;
    std::vector<T> accum_edge_off;
    std::vector<T> accum_iso_v_num;
    T mem_block_1_st;
    T mem_block_2_st;
public:
    template<typename U, typename DstU>
    explicit Memory(Graph<U, DstU> const &graph)
        : bmp{cal_mem_size<T, Graph<U, DstU>>(graph)/cacheline_edge_num},
        mem_block_1_st{0}, mem_block_2_st{get_aligned_size<T>(graph.get_vertex_number())} {
        bmp.reset();
        load_graph(graph);
    }
    template<typename U, typename DstU>
    void load_graph(Graph<U, DstU> const &graph);
    template<typename U, typename V>
    int access(U vid, V offset);
    template<typename U, typename V>
    T get_addr(U vid, V offset);
    void reset() {
        bmp.reset();
    }
    template<typename U, typename DstU>
    void check(Graph<U, DstU> const &graph);
};

template<typename T>
    template<typename U, typename DstU>
void Memory<T>::load_graph(Graph<U, DstU> const &graph) {
    accum_iso_v_num.reserve(graph.get_vertex_number() + 1);
    accum_edge_off.reserve(graph.get_vertex_number() + 1);
    accum_iso_v_num.emplace_back(0);
    accum_edge_off.emplace_back(0);
    for (auto const u : std::views::iota(0, graph.get_vertex_number())) {
        accum_edge_off.emplace_back(
            (graph.in_degree(u) > 0) ? (graph.in_degree(u) - 1) : 0);
    }
    for (auto const u : std::views::iota(0, graph.get_vertex_number())) {
        accum_iso_v_num.emplace_back(graph.in_degree(u) == 0);
    }
    std::partial_sum(accum_edge_off.begin(), accum_edge_off.end(), accum_edge_off.begin(), std::plus<>());
    std::partial_sum(accum_iso_v_num.begin(), accum_iso_v_num.end(), accum_iso_v_num.begin(), std::plus<>());
}

template<typename T>
    template<typename U, typename V>
T Memory<T>::get_addr(U vid, V offset) {
    return (offset == 0) ?
           (mem_block_1_st + vid - accum_iso_v_num[vid])
           : (mem_block_2_st + accum_edge_off[vid] + offset - 1);
}

template<typename T>
    template<typename U, typename V>
int Memory<T>::access(U vid, V offset) {
    T cacheline_id = get_addr(vid, offset) / cacheline_edge_num;
    int edge_visited{};
    if (!bmp.get_bit(cacheline_id)) {
        bmp.set_bit(cacheline_id);
        edge_visited = 16;
    } else {
        edge_visited = 0; // or 1?
    }
    return edge_visited;
}

template<typename T>
template<typename U, typename DstU>
void Memory<T>::check(Graph<U, DstU> const &graph) {
    std::vector<int> mem_addr(10000000, 0);
    int now_mem_addr_1 = mem_block_1_st;
    int now_mem_addr_2 = mem_block_2_st;
    for (int i = 0; i < graph.get_vertex_number(); ++i) {
        int tmp_e_id = graph.get_in_offset()[i];
        for (int j = 0; j < graph.in_degree(i); ++j) {
            if (j < 1) {
                mem_addr[tmp_e_id] = now_mem_addr_1;
                now_mem_addr_1++;
            } else {
                mem_addr[tmp_e_id] = now_mem_addr_2;
                now_mem_addr_2++;
            }
            tmp_e_id++;
        }
    }
    int tmp_e_id{};
    for (int v = 0; v < graph.get_vertex_number(); ++v) {
        int offset{};
        for (auto const &u : graph.in_neighbors(v)) {
            assert(get_addr(v, offset) == mem_addr[tmp_e_id]);
            tmp_e_id++;
            offset++;
        }
    }
}

#endif //EXPERIMENT_MEMORY_H
