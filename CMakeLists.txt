cmake_minimum_required(VERSION 4.3)
project(Genome_Project)

set(CMAKE_CXX_STANDARD 23)

add_executable(stat stat.cpp)
add_executable(main main.cpp)
add_executable(analysis analysis.cpp)
