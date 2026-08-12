cmake_minimum_required(VERSION 3.20)
project(dbx4 LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

enable_testing()

# ALL production source files
add_library(dbx4_core STATIC
  src/advanced_features.cpp
  src/advanced_sql_executor.cpp
  src/agent_integration.cpp
  src/database.cpp
  src/database_engine.cpp
  src/durable_log_system.cpp
  src/indexing.cpp
  src/mvcc_locks.cpp
  src/query_executor.cpp
  src/query_executor_engine.cpp
  src/query_executor_public_api.cpp
  src/recovery_engine_complete.cpp
  src/recovery_engine_production.cpp
  src/recovery_wal.cpp
  src/recovery_wal_hardened.cpp
  src/sql_executor_enhanced.cpp
  src/sql_lexer.cpp
  src/sql_parser.cpp
  src/sql_parser_enhanced.cpp
  src/storage_complete.cpp
  src/storage_engine_production.cpp
  src/storage_engine_test_fixed.cpp
  src/testing_suite.cpp
  src/transaction_correctness.cpp
  src/transaction_executor.cpp
  src/transaction_integrity_e2e.cpp
  src/transaction_manager.cpp
  src/wal_codec_robust.cpp
)

target_compile_options(dbx4_core PRIVATE -Wall -Wextra -fPIC)
target_include_directories(dbx4_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Build all tests
file(GLOB TEST_FILES "tests/*.cpp")
foreach(test_file ${TEST_FILES})
  get_filename_component(test_name ${test_file} NAME_WE)
  add_executable(${test_name} ${test_file})
  target_link_libraries(${test_name} dbx4_core)
  target_include_directories(${test_name} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
  add_test(NAME ${test_name} COMMAND ${test_name} TIMEOUT 120)
endforeach()
