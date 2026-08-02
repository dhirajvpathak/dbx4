cmake_minimum_required(VERSION 3.20)
project(DBX4 VERSION 1.0.0 LANGUAGES CXX)

# ============================================================================
# PROJECT SETTINGS
# ============================================================================

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS_RELEASE "/O2 /W4 ")
set(CMAKE_CXX_FLAGS_DEBUG "/Zi /Od /RTC1")

# ============================================================================
# COMPILER SETTINGS FOR MSVC
# ============================================================================

if(MSVC)
    # Visual Studio specific settings
    add_compile_options(/MP)  # Multi-processor compilation
    add_compile_options(/W4)  # Warning level 4
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

# ============================================================================
# SOURCE FILES
# ============================================================================

set(SOURCES_PHASE1 src/storage_complete.cpp)
set(SOURCES_PHASE2 src/mvcc_locks.cpp)
set(SOURCES_PHASE3 src/recovery_wal.cpp)
set(SOURCES_PHASE4 src/indexing.cpp)
set(SOURCES_PHASE5 src/advanced_features.cpp)
set(SOURCES_PHASE6 src/testing_suite.cpp)

# ============================================================================
# PHASE 1: STORAGE ENGINE
# ============================================================================

add_executable(storage_complete ${SOURCES_PHASE1})

target_include_directories(storage_complete PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(storage_complete PROPERTIES
    OUTPUT_NAME storage_complete
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(storage_complete PRIVATE /EHsc)
endif()

# ============================================================================
# PHASE 2: MVCC + LOCKS
# ============================================================================

add_executable(mvcc_locks ${SOURCES_PHASE2})

target_include_directories(mvcc_locks PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(mvcc_locks PROPERTIES
    OUTPUT_NAME mvcc_locks
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(mvcc_locks PRIVATE /EHsc)
endif()

# ============================================================================
# PHASE 3: RECOVERY + WAL
# ============================================================================

add_executable(recovery_wal ${SOURCES_PHASE3})

target_include_directories(recovery_wal PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(recovery_wal PROPERTIES
    OUTPUT_NAME recovery_wal
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(recovery_wal PRIVATE /EHsc)
endif()

# ============================================================================
# PHASE 4: INDEXING
# ============================================================================

add_executable(indexing ${SOURCES_PHASE4})

target_include_directories(indexing PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(indexing PROPERTIES
    OUTPUT_NAME indexing
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(indexing PRIVATE /EHsc)
endif()

# ============================================================================
# PHASE 5: ADVANCED FEATURES
# ============================================================================

add_executable(advanced_features ${SOURCES_PHASE5})

target_include_directories(advanced_features PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(advanced_features PROPERTIES
    OUTPUT_NAME advanced_features
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(advanced_features PRIVATE /EHsc)
endif()

# ============================================================================
# PHASE 6: TESTING SUITE
# ============================================================================

add_executable(testing_suite ${SOURCES_PHASE6})

target_include_directories(testing_suite PRIVATE ${CMAKE_SOURCE_DIR}/include)

set_target_properties(testing_suite PROPERTIES
    OUTPUT_NAME testing_suite
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
    VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

if(MSVC)
    target_compile_options(testing_suite PRIVATE /EHsc)
endif()

# ============================================================================
# ENABLE TESTING
# ============================================================================

enable_testing()

# Add tests for each phase
add_test(
    NAME Phase1_StorageEngine
    COMMAND storage_complete
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

add_test(
    NAME Phase2_MVCCLocks
    COMMAND mvcc_locks
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

add_test(
    NAME Phase3_RecoveryWAL
    COMMAND recovery_wal
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

add_test(
    NAME Phase4_Indexing
    COMMAND indexing
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

add_test(
    NAME Phase5_AdvancedFeatures
    COMMAND advanced_features
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

add_test(
    NAME Phase6_TestingSuite
    COMMAND testing_suite
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

# ============================================================================
# PHASE 4: SQL EXECUTOR TEST SUITE
# ============================================================================

# SQL Executor Tests
add_executable(sql_executor_tests tests/sql_executor_tests.cpp src/query_executor.cpp)
target_include_directories(sql_executor_tests PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(sql_executor_tests PRIVATE /std:c++17 /W4)

add_test(
    NAME SQLExecutorTests
    COMMAND sql_executor_tests
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)

# SQL Executor Integration Tests
add_executable(sql_executor_integration tests/sql_executor_integration_test.cpp)
target_include_directories(sql_executor_integration PRIVATE ${CMAKE_SOURCE_DIR}/include)
target_compile_options(sql_executor_integration PRIVATE /std:c++17 /W4)

add_test(
    NAME SQLExecutorIntegrationTests
    COMMAND sql_executor_integration
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/$<CONFIG>
)


# ============================================================================
# BUILD OUTPUT MESSAGE
# ============================================================================

message(STATUS "")
message(STATUS "========================================")
message(STATUS "DBX4 Database Engine Build Configuration")
message(STATUS "========================================")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Compiler: ${CMAKE_CXX_COMPILER_ID}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "")
message(STATUS "Executables will be generated in:")
message(STATUS "  - ${CMAKE_BINARY_DIR}/Release/")
message(STATUS "  - ${CMAKE_BINARY_DIR}/Debug/")
message(STATUS "")
message(STATUS "To build:")
message(STATUS "  cmake --build . --config Release -j8")
message(STATUS "")
message(STATUS "To run tests:")
message(STATUS "  ctest -C Release -V")
message(STATUS "")
message(STATUS "========================================")
message(STATUS "")



