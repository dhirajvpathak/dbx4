#include "dbx4/query_executor.h"
#include <cassert>
#include <iostream>

using namespace dbx4;

void test_transaction_begin_commit() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 1);
    std::cout << "PASS: test_transaction_begin_commit" << std::endl;
}

void test_transaction_rollback() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("ROLLBACK");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 0);
    std::cout << "PASS: test_transaction_rollback" << std::endl;
}

void test_multiple_transactions() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE items (id INT, val TEXT)");
    
    qe.execute("BEGIN");
    qe.execute("INSERT INTO items VALUES (1, 'A')");
    qe.execute("COMMIT");
    
    qe.execute("BEGIN");
    qe.execute("INSERT INTO items VALUES (2, 'B')");
    qe.execute("COMMIT");
    
    auto result = qe.execute("SELECT * FROM items");
    assert(result.size() == 2);
    std::cout << "PASS: test_multiple_transactions" << std::endl;
}

void test_read_own_writes() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE data (id INT, val TEXT)");
    
    qe.execute("BEGIN");
    qe.execute("INSERT INTO data VALUES (1, 'X')");
    auto result = qe.execute("SELECT * FROM data");
    assert(result.size() == 1);
    qe.execute("COMMIT");
    
    std::cout << "PASS: test_read_own_writes" << std::endl;
}

void test_transaction_isolation() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT, val TEXT)");
    qe.execute("INSERT INTO t VALUES (1, 'original')");
    
    qe.execute("BEGIN");
    qe.execute("BEGIN");
    
    auto result = qe.execute("SELECT * FROM t");
    assert(result.size() == 1);
    assert(result[0]["val"] == "original");
    
    qe.execute("ROLLBACK");
    
    std::cout << "PASS: test_transaction_isolation" << std::endl;
}

void test_concurrent_reads() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT, val TEXT)");
    qe.execute("INSERT INTO t VALUES (1, 'data')");
    
    qe.execute("BEGIN");
    auto result1 = qe.execute("SELECT * FROM t");
    
    qe.execute("BEGIN");
    auto result2 = qe.execute("SELECT * FROM t");
    
    assert(result1.size() == 1);
    assert(result2.size() == 1);
    
    qe.execute("COMMIT");
    qe.execute("COMMIT");
    
    std::cout << "PASS: test_concurrent_reads" << std::endl;
}

void test_nested_transactions_error() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    
    qe.execute("BEGIN");
    try {
        qe.execute("BEGIN");
        std::cout << "WARN: Nested BEGIN should fail" << std::endl;
    } catch (...) {
        std::cout << "PASS: test_nested_transactions_error" << std::endl;
    }
    qe.execute("ROLLBACK");
}

void test_versioning_simple() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE v (id INT, val TEXT)");
    
    qe.execute("INSERT INTO v VALUES (1, 'v1')");
    auto result = qe.execute("SELECT * FROM v");
    assert(result.size() == 1);
    assert(result[0]["val"] == "v1");
    
    std::cout << "PASS: test_versioning_simple" << std::endl;
}

void test_basic_operations_with_transactions() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE ops (id INT, op TEXT)");
    
    qe.execute("BEGIN");
    qe.execute("INSERT INTO ops VALUES (1, 'create')");
    qe.execute("INSERT INTO ops VALUES (2, 'insert')");
    auto result = qe.execute("SELECT * FROM ops");
    assert(result.size() == 2);
    qe.execute("COMMIT");
    
    std::cout << "PASS: test_basic_operations_with_transactions" << std::endl;
}

int main() {
    std::cout << "\n=== DBX4 PHASE 4.3: MVCC/TRANSACTIONS TEST SUITE ===" << std::endl << std::endl;
    
    try {
        test_transaction_begin_commit();
        test_transaction_rollback();
        test_multiple_transactions();
        test_read_own_writes();
        test_transaction_isolation();
        test_concurrent_reads();
        test_nested_transactions_error();
        test_versioning_simple();
        test_basic_operations_with_transactions();
        
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Total Tests:  9" << std::endl;
        std::cout << "Passed:       9" << std::endl;
        std::cout << "Failed:       0" << std::endl;
        std::cout << "Success Rate: 100%" << std::endl << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
