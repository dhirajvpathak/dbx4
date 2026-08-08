cmake_minimum_required(VERSION 3.20)
project(DBX4 VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# MSVC-specific compiler flags
if(MSVC)
    add_compile_options(/MP /W4)
    set(CMAKE_CXX_FLAGS_RELEASE "/O2 /W4")
    set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /RTC1")
else()
    # GCC and Clang flags
    add_compile_options(-Wall -Wextra -Wpedantic)
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O0 -D_DEBUG")
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include/dbx4)

set(DBX4_CORE_SOURCES
    src/advanced_features.cpp
    src/agent_integration.cpp
    src/durable_log_system.cpp
    src/indexing.cpp
    src/mvcc_locks.cpp
    src/query_executor.cpp
    src/query_executor_engine.cpp
    src/recovery_wal.cpp
    src/sql_executor_enhanced.cpp
    src/sql_lexer.cpp
    src/sql_parser.cpp
    src/sql_parser_enhanced.cpp
    src/storage_complete.cpp
    src/storage_engine_production.cpp
    src/testing_suite.cpp
    src/transaction_correctness.cpp
    src/transaction_manager.cpp
)

add_library(dbx4_core STATIC ${DBX4_CORE_SOURCES})
target_include_directories(dbx4_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

if(MSVC)
    target_compile_options(dbx4_core PRIVATE /WX)
else()
    target_compile_options(dbx4_core PRIVATE -Werror)
endif()
