cmake_minimum_required(VERSION 3.26)
project(benchmarks)

set(CMAKE_BUILD_TYPE Release)

set(CMAKE_CXX_FLAGS "-Wall -Wextra")
set(CMAKE_CXX_FLAGS_DEBUG "-g")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

set(CMAKE_CXX_STANDARD 20)

add_executable(benchmarks ../include/xtree.hpp ../include/xtree.cpp main.cpp)
