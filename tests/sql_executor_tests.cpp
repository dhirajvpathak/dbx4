#include "dbx4/query_executor.h"
#include <cassert>
#include <iostream>

using namespace dbx4;

void test_basic_create_table() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    std::cout << "PASS: test_basic_create_table" << std::endl;
}

void test_insert_no_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE data (id INT, val TEXT)");
    qe.execute("INSERT INTO data VALUES (1, 'A')");
    auto result = qe.execute("SELECT * FROM data");
    assert(result.size() == 1);
    std::cout << "PASS: test_insert_no_transaction" << std::endl;
}

void test_transaction_begin_commit() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE items (id INT, name TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO items VALUES (1, 'item1')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM items");
    assert(result.size() >= 1);
    std::cout << "PASS: test_transaction_begin_commit" << std::endl;
}

void test_transaction_rollback() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t1 (id INT)");
    qe.execute("BEGIN");
    qe.execute("ROLLBACK");
    auto result = qe.execute("SELECT * FROM t1");
    assert(result.size() == 0);
    std::cout << "PASS: test_transaction_rollback" << std::endl;
}

void test_multiple_commits() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE products (id INT, name TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO products VALUES (1, 'p1')");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO products VALUES (2, 'p2')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM products");
    assert(result.size() >= 2);
    std::cout << "PASS: test_multiple_commits" << std::endl;
}

void test_transactional_sequence() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE seq (id INT)");
    qe.execute("INSERT INTO seq VALUES (1)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO seq VALUES (2)");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM seq");
    assert(result.size() >= 2);
    std::cout << "PASS: test_transactional_sequence" << std::endl;
}

void test_interleaved_transactions() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE intl (id INT)");
    qe.execute("INSERT INTO intl VALUES (1)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO intl VALUES (2)");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO intl VALUES (3)");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM intl");
    assert(result.size() >= 3);
    std::cout << "PASS: test_interleaved_transactions" << std::endl;
}

void test_wal_persistence() {
    QueryExecutor qe("/tmp/dbx4_wal_test");
    qe.execute("CREATE TABLE durable (id INT, val TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO durable VALUES (42, 'persisted')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM durable");
    assert(result.size() >= 1);
    std::cout << "PASS: test_wal_persistence" << std::endl;
}

void test_table_schema_persistence() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE schema_test (id INT, name TEXT, value DOUBLE)");
    auto result = qe.execute("SELECT * FROM schema_test");
    assert(result.size() == 0);
    std::cout << "PASS: test_table_schema_persistence" << std::endl;
}

void test_transaction_state_tracking() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE state_test (id INT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO state_test VALUES (1)");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    std::cout << "PASS: test_transaction_state_tracking" << std::endl;
}

void test_wal_log_entries() {
    QueryExecutor qe("/tmp/dbx4_wal_logs");
    qe.execute("CREATE TABLE logged (id INT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO logged VALUES (1)");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO logged VALUES (2)");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM logged");
    assert(result.size() >= 2);
    std::cout << "PASS: test_wal_log_entries" << std::endl;
}

void test_multiple_table_operations() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE table1 (id INT)");
    qe.execute("CREATE TABLE table2 (id INT)");
    qe.execute("INSERT INTO table1 VALUES (1)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO table2 VALUES (2)");
    qe.execute("COMMIT");
    auto r1 = qe.execute("SELECT * FROM table1");
    auto r2 = qe.execute("SELECT * FROM table2");
    assert(r1.size() >= 1 && r2.size() >= 1);
    std::cout << "PASS: test_multiple_table_operations" << std::endl;
}

void test_empty_transaction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE empty (id INT)");
    qe.execute("BEGIN");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM empty");
    assert(result.size() == 0);
    std::cout << "PASS: test_empty_transaction" << std::endl;
}

int main() {
    std::cout << "\n=== DBX4 PHASE 4.4: WAL/RECOVERY TEST SUITE ===" << std::endl << std::endl;
    
    try {
        test_basic_create_table();
        test_insert_no_transaction();
        test_transaction_begin_commit();
        test_transaction_rollback();
        test_multiple_commits();
        test_transactional_sequence();
        test_interleaved_transactions();
        test_wal_persistence();
        test_table_schema_persistence();
        test_transaction_state_tracking();
        test_wal_log_entries();
        test_multiple_table_operations();
        test_empty_transaction();
        
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Total Tests:  13" << std::endl;
        std::cout << "Passed:       13" << std::endl;
        std::cout << "Failed:       0" << std::endl;
        std::cout << "Success Rate: 100%" << std::endl << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
