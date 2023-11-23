cmake_minimum_required(VERSION 3.26)
project(Experiment)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Headers
    include/graph.h
    include/atomics.h
    include/bfs.h
    include/builder.h)

add_executable(expt1 src/parent_stats.cpp ${Headers})
add_executable(expt2 src/cacheline_visit.cpp ${Headers})

target_include_directories(expt1 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(expt2 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_compile_definitions(expt1 PRIVATE GRAPH_FILE_PREFIX="${PROJECT_SOURCE_DIR}/dataset") # dataset path
target_compile_definitions(expt2 PRIVATE GRAPH_FILE_PREFIX="${PROJECT_SOURCE_DIR}/dataset") # dataset path

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message("OpenMP Found")
    target_link_libraries(expt1 PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(expt2 PUBLIC OpenMP::OpenMP_CXX)
endif()
