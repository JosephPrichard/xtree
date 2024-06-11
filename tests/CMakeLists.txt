cmake_minimum_required(VERSION 3.26)
project(xtree-test)

set(CMAKE_CXX_STANDARD 20)

add_executable(xtree-test ../include/xml.cpp ../include/xml.hpp main.cpp)