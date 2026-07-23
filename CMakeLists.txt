cmake_minimum_required(VERSION 3.15)
project(DBX4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-g -O0")

# Project structure
add_subdirectory(src)
add_subdirectory(tests)
add_subdirectory(benchmarks)

# Main executable (once src is ready)
# add_executable(dbx4_server src/main.cpp)

