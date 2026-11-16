cmake_minimum_required(VERSION 4.3)
project(Genome_Project)

set(CMAKE_CXX_STANDARD 23)

find_package(OpenMP REQUIRED COMPONENTS CXX)

add_executable(stat stat.cpp)
add_executable(exact exact.cpp)
add_executable(wildcards wildcards.cpp)
add_executable(analysis analysis.cpp)

target_link_libraries(exact PRIVATE OpenMP::OpenMP_CXX)
target_link_libraries(wildcards PRIVATE OpenMP::OpenMP_CXX)

# FindOpenMP can miss MinGW's runtime libraries when the compiler path contains spaces
if(MINGW AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT OpenMP_CXX_LIBRARIES)
	target_link_options(exact PRIVATE -fopenmp)
	target_link_options(wildcards PRIVATE -fopenmp)
endif()
