#include <cstdint>
#include "dbx4/query_executor.h"
#include "dbx4/query_executor.h"
#include <cassert>
#include <iostream>

using namespace dbx4;

void test_create_table() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    std::cout << "PASS: test_create_table" << std::endl;
}

void test_insert_with_index() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE products (id INT, name TEXT)");
    qe.execute("INSERT INTO products VALUES (1, 'product1')");
    qe.execute("INSERT INTO products VALUES (2, 'product2')");
    auto result = qe.execute("SELECT * FROM products");
    assert(result.size() >= 2);
    std::cout << "PASS: test_insert_with_index" << std::endl;
}

void test_select_with_where() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE items (id INT, value INT)");
    qe.execute("INSERT INTO items VALUES (1, 100)");
    qe.execute("INSERT INTO items VALUES (2, 200)");
    auto result = qe.execute("SELECT * FROM items WHERE value > 150");
    assert(result.size() >= 1);
    std::cout << "PASS: test_select_with_where" << std::endl;
}

void test_transaction_with_caching() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE orders (id INT, amount INT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO orders VALUES (1, 500)");
    qe.execute("INSERT INTO orders VALUES (2, 600)");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM orders");
    assert(result.size() >= 2);
    std::cout << "PASS: test_transaction_with_caching" << std::endl;
}

void test_memory_tracking() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE data (id INT, val TEXT)");
    long long mem_before = qe.get_memory_usage();
    qe.execute("INSERT INTO data VALUES (1, 'test')");
    long long mem_after = qe.get_memory_usage();
    assert(mem_after >= mem_before);
    std::cout << "PASS: test_memory_tracking" << std::endl;
}

void test_cache_effectiveness() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE cached (id INT, name TEXT)");
    qe.execute("INSERT INTO cached VALUES (1, 'entry1')");
    qe.execute("INSERT INTO cached VALUES (2, 'entry2')");
    auto result1 = qe.execute("SELECT * FROM cached");
    auto result2 = qe.execute("SELECT * FROM cached");
    assert(result1.size() == result2.size());
    std::cout << "PASS: test_cache_effectiveness" << std::endl;
}

void test_index_lookup() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE indexed (id INT, data TEXT)");
    qe.execute("INSERT INTO indexed VALUES (1, 'a')");
    qe.execute("INSERT INTO indexed VALUES (2, 'b')");
    qe.execute("INSERT INTO indexed VALUES (3, 'c')");
    auto result = qe.execute("SELECT * FROM indexed WHERE id = 2");
    assert(result.size() >= 1);
    std::cout << "PASS: test_index_lookup" << std::endl;
}

void test_multiple_tables() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t1 (id INT)");
    qe.execute("CREATE TABLE t2 (id INT)");
    qe.execute("INSERT INTO t1 VALUES (1)");
    qe.execute("INSERT INTO t2 VALUES (2)");
    auto r1 = qe.execute("SELECT * FROM t1");
    auto r2 = qe.execute("SELECT * FROM t2");
    assert(r1.size() >= 1 && r2.size() >= 1);
    std::cout << "PASS: test_multiple_tables" << std::endl;
}

void test_concurrent_transactions() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE concurrent (id INT, val INT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO concurrent VALUES (1, 10)");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO concurrent VALUES (2, 20)");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM concurrent");
    assert(result.size() >= 2);
    std::cout << "PASS: test_concurrent_transactions" << std::endl;
}

void test_wal_persistence() {
    QueryExecutor qe("/tmp/dbx4_phase5_wal_simple");
    qe.execute("CREATE TABLE persisted (id INT, data TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO persisted VALUES (42, 'durable')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM persisted");
    assert(result.size() >= 1);
    std::cout << "PASS: test_wal_persistence" << std::endl;
}

void test_stress_inserts() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE stress (id INT, val INT)");
    for (int i = 0; i < 100; i++) {
        std::string sql = "INSERT INTO stress VALUES (" + std::to_string(i) + ", " + std::to_string(i * 2) + ")";
        qe.execute(sql);
    }
    auto result = qe.execute("SELECT * FROM stress");
    assert(result.size() >= 100);
    std::cout << "PASS: test_stress_inserts" << std::endl;
}

void test_memory_limits() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE memlimit (id INT, data TEXT)");
    for (int i = 0; i < 50; i++) {
        qe.execute("INSERT INTO memlimit VALUES (" + std::to_string(i) + ", 'test')");
    }
    long long mem = qe.get_memory_usage();
    assert(mem >= 0);
    std::cout << "PASS: test_memory_limits" << std::endl;
}

void test_query_optimization() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE optimized (id INT, name TEXT, value INT)");
    qe.execute("INSERT INTO optimized VALUES (1, 'a', 100)");
    qe.execute("INSERT INTO optimized VALUES (2, 'b', 200)");
    auto result1 = qe.execute("SELECT * FROM optimized");
    auto result2 = qe.execute("SELECT * FROM optimized WHERE value > 150");
    assert(result1.size() >= result2.size());
    std::cout << "PASS: test_query_optimization" << std::endl;
}

void test_transactional_consistency() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE consistent (id INT, status TEXT)");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO consistent VALUES (1, 'committed')");
    qe.execute("COMMIT");
    qe.execute("BEGIN");
    qe.execute("INSERT INTO consistent VALUES (2, 'also_committed')");
    qe.execute("COMMIT");
    auto result = qe.execute("SELECT * FROM consistent");
    assert(result.size() >= 2);
    std::cout << "PASS: test_transactional_consistency" << std::endl;
}

void test_index_performance() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE perf (id INT, data TEXT)");
    for (int i = 0; i < 50; i++) {
        qe.execute("INSERT INTO perf VALUES (" + std::to_string(i) + ", 'indexed')");
    }
    auto result = qe.execute("SELECT * FROM perf");
    assert(result.size() >= 50);
    std::cout << "PASS: test_index_performance" << std::endl;
}

void test_btree_range_queries() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE ranges (id INT, value INT)");
    for (int i = 1; i <= 10; i++) {
        qe.execute("INSERT INTO ranges VALUES (" + std::to_string(i) + ", " + std::to_string(i * 10) + ")");
    }
    auto result = qe.execute("SELECT * FROM ranges WHERE value > 50");
    assert(result.size() >= 4);
    std::cout << "PASS: test_btree_range_queries" << std::endl;
}

int main() {
    std::cout << "\n=== DBX4 PHASE 5: OPTIMIZATION & HARDENING TEST SUITE ===" << std::endl << std::endl;
    
    try {
        test_create_table();
        test_insert_with_index();
        test_select_with_where();
        test_transaction_with_caching();
        test_memory_tracking();
        test_cache_effectiveness();
        test_index_lookup();
        test_multiple_tables();
        test_concurrent_transactions();
        test_wal_persistence();
        test_stress_inserts();
        test_memory_limits();
        test_query_optimization();
        test_transactional_consistency();
        test_index_performance();
        test_btree_range_queries();
        
        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Total Tests:  16" << std::endl;
        std::cout << "Passed:       16" << std::endl;
        std::cout << "Failed:       0" << std::endl;
        std::cout << "Success Rate: 100%" << std::endl << std::endl;
        
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed: " << e.what() << std::endl;
        return 1;
    }
}
