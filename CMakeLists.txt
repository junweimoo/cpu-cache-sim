cmake_minimum_required(VERSION 3.29)
project(cpu_cache_sim)

set(CMAKE_CXX_STANDARD 20)

file(GLOB_RECURSE SOURCES "src/*.cpp")

add_executable(cpu_cache_sim ${SOURCES})
