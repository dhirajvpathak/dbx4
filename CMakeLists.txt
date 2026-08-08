cmake_minimum_required(VERSION 3.20)
project(dbx4 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")

# Enable testing
enable_testing()

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

# Core library sources
set(DBX4_CORE_SOURCES
    src/storage_engine_production.cpp
    src/recovery_wal.cpp
    src/transaction_manager.cpp
    src/query_executor_engine.cpp
    src/query_executor.cpp
    src/mvcc_locks.cpp
    src/sql_parser.cpp
    src/sql_executor_enhanced.cpp
    src/indexing.cpp
    src/concurrency.cpp
    src/redo_log.cpp
    src/snapshot_isolation.cpp
    src/page_manager.cpp
    src/buffer_pool.cpp
    src/agent_integration.cpp
    src/durable_log_system.cpp
    src/sql_parser_enhanced.cpp
)

# Create core library
add_library(dbx4_core STATIC ${DBX4_CORE_SOURCES})

# ============================================================================
# TEST TARGETS - Registered with CTest
# ============================================================================

# Test 1: Model A Synthetic Scenarios
add_executable(test_model_a_synthetic tests/model_a_synthetic_scenarios.cpp)
target_link_libraries(test_model_a_synthetic dbx4_core)
add_test(NAME ModelA_SyntheticScenarios COMMAND test_model_a_synthetic)

# Test 2: Model B Workload Replay
add_executable(test_model_b_workload tests/model_b_workload_replay.cpp)
target_link_libraries(test_model_b_workload dbx4_core)
add_test(NAME ModelB_WorkloadReplay COMMAND test_model_b_workload)

# Test 3: Model B End-to-End (Hardened Version)
add_executable(test_model_b_e2e tests/model_b_end_to_end_v2.cpp)
target_link_libraries(test_model_b_e2e dbx4_core)
add_test(NAME ModelB_EndToEnd COMMAND test_model_b_e2e)

# Test 4: Hardened WAL Parser
add_executable(test_hardened_wal src/recovery_wal_hardened.cpp)
target_link_libraries(test_hardened_wal dbx4_core)
add_test(NAME HardenedWALParser COMMAND test_hardened_wal)

# Test 5: Transaction Integrity Verification
add_executable(test_txn_integrity src/transaction_integrity_e2e.cpp)
target_link_libraries(test_txn_integrity dbx4_core)
add_test(NAME TransactionIntegrity COMMAND test_txn_integrity)

# ============================================================================
# ENGINE VERIFICATION TESTS (Against actual QueryExecutor)
# ============================================================================

# Test 6: Real recovery test (uses actual engine)
add_executable(test_real_recovery_engine tests/real_recovery_engine_test.cpp)
target_link_libraries(test_real_recovery_engine dbx4_core)
add_test(NAME RealRecoveryEngine COMMAND test_real_recovery_engine)

# Test 7: Query execution test (uses actual engine)
add_executable(test_query_execution tests/query_execution_test.cpp)
target_link_libraries(test_query_execution dbx4_core)
add_test(NAME QueryExecution COMMAND test_query_execution)

# ============================================================================
# CI GATES
# ============================================================================

# Verify all tests are registered
message(STATUS "CTest: ${CMAKE_CTEST_COMMAND} -N")
message(STATUS "Run tests: ${CMAKE_CTEST_COMMAND} -V")
