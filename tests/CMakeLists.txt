cmake_minimum_required(VERSION 3.26)
project(xmldom-test)

set(CMAKE_CXX_STANDARD 20)

add_executable(xmldom-test ../include/xml.cpp ../include/xml.hpp main.cpp)