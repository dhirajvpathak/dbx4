cmake_minimum_required(VERSION 3.20)
project(dbx4 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

enable_testing()

# MINIMAL core - pick ONE version of each component
add_library(dbx4_core STATIC
  src/query_executor_engine.cpp
  src/storage_engine_production.cpp
  src/recovery_wal_hardened.cpp
  src/mvcc_locks.cpp
  src/transaction_manager.cpp
  src/wal_codec_robust.cpp
)

target_compile_options(dbx4_core PRIVATE -Wall -Wextra -fPIC)

file(GLOB TEST_FILES "tests/*.cpp")

foreach(test_file ${TEST_FILES})
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} dbx4_core)
  add_test(NAME ${test_name} COMMAND ${test_name} TIMEOUT 120)
endforeach()
