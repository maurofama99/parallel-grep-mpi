cmake_minimum_required(VERSION 3.20)
project (parallel_grep)

set(CMAKE_CXX_STANDARD 14)

# Add MPI Package to Project
find_package(MPI REQUIRED)

add_executable(parallel_grep main.cpp grep.h grep.cpp)
# Add libraries for code completion and compiling
target_link_libraries(parallel_grep PUBLIC MPI::MPI_CXX)
