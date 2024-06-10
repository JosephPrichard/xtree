cmake_minimum_required(VERSION 3.26)

project(xmldom)
set(CMAKE_BUILD_TYPE Release)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
add_library(xmldom SHARED xml.cpp xml.hpp)