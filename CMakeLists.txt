cmake_minimum_required(VERSION 3.12)
project(Experiment)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(Headers1
    include/graph.h
    include/atomics.h
    include/bfs.h
    include/builder.h
    include/utils.h)

set(Headers2
        include/graph.h
        include/atomics.h
        include/builder.h
        include/memory.h
        include/bitmap.h
        include/bfs.h
        include/utils.h)

set(SubModuleHeaders
    plf_nanotimer/plf_nanotimer.h)

add_executable(expt1 src/parent_stats.cpp ${Headers1} ${SubModuleHeaders})
add_executable(expt2 src/cacheline_visit.cpp ${Headers2} ${SubModuleHeaders})

target_include_directories(expt1 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(expt1 PRIVATE ${PROJECT_SOURCE_DIR}/plf_nanotimer)
target_include_directories(expt2 PRIVATE ${PROJECT_SOURCE_DIR}/include)
target_include_directories(expt2 PRIVATE ${PROJECT_SOURCE_DIR}/plf_nanotimer)
target_compile_definitions(expt1 PRIVATE GRAPH_FILE_PREFIX="${PROJECT_SOURCE_DIR}/dataset") # dataset path
target_compile_definitions(expt2 PRIVATE GRAPH_FILE_PREFIX="${PROJECT_SOURCE_DIR}/dataset") # dataset path

find_package(OpenMP)
if(OpenMP_CXX_FOUND)
    message("OpenMP Found")
    target_link_libraries(expt1 PUBLIC OpenMP::OpenMP_CXX)
    target_link_libraries(expt2 PUBLIC OpenMP::OpenMP_CXX)
endif()

add_executable(test_cpp src/test_cpp.cpp include/utils.h)
target_include_directories(test_cpp PRIVATE ${PROJECT_SOURCE_DIR}/include)
