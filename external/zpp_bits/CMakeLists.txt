cmake_minimum_required(VERSION 3.11 FATAL_ERROR)

add_library(zpp_bits INTERFACE)
target_include_directories(zpp_bits INTERFACE "${CMAKE_CURRENT_LIST_DIR}")
target_compile_features(zpp_bits INTERFACE cxx_std_20)

