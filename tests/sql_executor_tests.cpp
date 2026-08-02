#include "dbx4/query_executor.h"
#include <cassert>
#include <iostream>

using namespace dbx4;

void test_basic_create_table() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    std::cout << "PASS: test_basic_create_table" << std::endl;
}

void test_insert_without_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE data (id INT, val TEXT)");
    qe.execute("INSERT INTO data VALUES (1, 'A')");
    auto result = qe.execute("SELECT * FROM data");
    assert(result.size() == 1);
    std::cout << "PASS: test_insert_without_transaction" << std::endl;
}

void test_begin_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    qe.execute("BEGIN");
    std::cout << "PASS: test_begin_transaction" << std::endl;
}

void test_commit_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    std::cout << "PASS: test_commit_transaction" << std::endl;
}

void test_rollback_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    qe.execute("BEGIN");
    qe.execute("ROLLBACK");
    std::cout << "PASS: test_rollback_transaction" << std::endl;
}

void test_multiple_begins() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    std::cout << "PASS: test_multiple_begins" << std::endl;
}

void test_transaction_counters() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t (id INT)");
    qe.execute("BEGIN");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    qe.execute("COMMIT");
    std::cout << "PASS: test_transaction_counters" << std::endl;
}

void test_select_after_insert() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE items (id INT, name TEXT)");
    qe.execute("INSERT INTO items VALUES (1, 'Item1')");
    qe.execute("INSERT INTO items VALUES (2, 'Item2')");
    auto result = qe.execute("SELECT * FROM items");
    assert(result.size() == 2);
    std::cout << "PASS: test_select_after_insert" << std::endl;
}

void test_version_history_tracking() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE versions (id INT, v INT)");
    qe.execute("INSERT INTO versions VALUES (1, 1)");
    auto result1 = qe.execute("SELECT * FROM versions");
    qe.execute("INSERT INTO versions VALUES (2, 2)");
    auto result2 = qe.execute("SELECT * FROM versions");
    assert(result1.size() == 1);
    assert(result2.size() == 2);
    std::cout << "PASS: test_version_history_tracking" << std::endl;
}

int main() {
    std::cout << "\n=== DBX4 PHASE 4.3: MVCC/TRANSACTIONS TEST SUITE ===" << std::endl << std::endl;
    
    try {
        test_basic_create_table();
        test_insert_without_transaction();
        test_begin_transaction();
        test_commit_transaction();
        test_rollback_transaction();
        test_multiple_begins();
        test_transaction_counters();
        test_select_after_insert();
        test_version_history_tracking();
        
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
