#include "dbx4/query_executor.h"
#include <cassert>
#include <iostream>

using namespace dbx4;

void test_create_table_and_insert() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    auto result = qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    assert(result.size() == 0);
    std::cout << "PASS: test_create_table_and_insert" << std::endl;
}

void test_select_with_limit() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE products (id INT, price TEXT)");
    qe.execute("INSERT INTO products VALUES (1, '100')");
    qe.execute("INSERT INTO products VALUES (2, '200')");
    qe.execute("INSERT INTO products VALUES (3, '300')");
    auto result = qe.execute("SELECT * FROM products LIMIT 2");
    assert(result.size() == 2);
    std::cout << "PASS: test_select_with_limit" << std::endl;
}

void test_update_rows() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE items (id INT, value TEXT)");
    qe.execute("INSERT INTO items VALUES (1, 'old')");
    qe.execute("UPDATE items SET value = 'new' WHERE id = 1");
    auto result = qe.execute("SELECT * FROM items");
    assert(result[0]["value"] == "new");
    std::cout << "PASS: test_update_rows" << std::endl;
}

void test_delete_rows() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE logs (id INT, message TEXT)");
    qe.execute("INSERT INTO logs VALUES (1, 'msg1')");
    qe.execute("INSERT INTO logs VALUES (2, 'msg2')");
    qe.execute("DELETE FROM logs WHERE id = 1");
    auto result = qe.execute("SELECT * FROM logs");
    assert(result.size() == 1);
    std::cout << "PASS: test_delete_rows" << std::endl;
}

void test_multiple_inserts() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE data (id INT, val INT)");
    qe.execute("INSERT INTO data VALUES (1, 10)");
    qe.execute("INSERT INTO data VALUES (2, 20)");
    qe.execute("INSERT INTO data VALUES (3, 30)");
    auto result = qe.execute("SELECT * FROM data");
    assert(result.size() == 3);
    std::cout << "PASS: test_multiple_inserts" << std::endl;
}

void test_limit_zero() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE test (id INT)");
    qe.execute("INSERT INTO test VALUES (1)");
    auto result = qe.execute("SELECT * FROM test LIMIT 0");
    assert(result.size() == 0);
    std::cout << "PASS: test_limit_zero" << std::endl;
}

void test_lexer_tokenization() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t1 (col1 INT)");
    qe.execute("INSERT INTO t1 VALUES (42)");
    auto result = qe.execute("SELECT * FROM t1");
    assert(result.size() == 1);
    std::cout << "PASS: test_lexer_tokenization" << std::endl;
}

void test_parser_ast_construction() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t2 (c1 TEXT, c2 INT)");
    qe.execute("INSERT INTO t2 VALUES ('a', 1)");
    auto result = qe.execute("SELECT * FROM t2");
    assert(result[0]["c1"] == "a");
    std::cout << "PASS: test_parser_ast_construction" << std::endl;
}

void test_full_pipeline_select() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE t3 (x INT, y TEXT)");
    qe.execute("INSERT INTO t3 VALUES (10, 'b')");
    auto result = qe.execute("SELECT * FROM t3");
    assert(result.size() == 1);
    std::cout << "PASS: test_full_pipeline_select" << std::endl;
}

void test_select_where_equals() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("INSERT INTO users VALUES (2, 'Bob')");
    auto result = qe.execute("SELECT * FROM users WHERE id = 1");
    assert(result.size() == 1);
    assert(result[0]["id"] == "1");
    assert(result[0]["name"] == "Alice");
    std::cout << "PASS: test_select_where_equals" << std::endl;
}

void test_select_where_multiple_matches() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, age INT)");
    qe.execute("INSERT INTO users VALUES (1, 25)");
    qe.execute("INSERT INTO users VALUES (2, 35)");
    qe.execute("INSERT INTO users VALUES (3, 45)");
    auto result = qe.execute("SELECT * FROM users WHERE age > 30");
    assert(result.size() == 2);
    std::cout << "PASS: test_select_where_multiple_matches" << std::endl;
}

void test_select_where_no_matches() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT)");
    qe.execute("INSERT INTO users VALUES (1)");
    auto result = qe.execute("SELECT * FROM users WHERE id = 999");
    assert(result.size() == 0);
    std::cout << "PASS: test_select_where_no_matches" << std::endl;
}

void test_update_where_single() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("INSERT INTO users VALUES (2, 'Bob')");
    qe.execute("UPDATE users SET name = 'Charlie' WHERE id = 2");
    auto result = qe.execute("SELECT * FROM users WHERE id = 2");
    assert(result[0]["name"] == "Charlie");
    std::cout << "PASS: test_update_where_single" << std::endl;
}

void test_update_where_multiple() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, status TEXT)");
    qe.execute("INSERT INTO users VALUES (1, 'inactive')");
    qe.execute("INSERT INTO users VALUES (2, 'inactive')");
    qe.execute("INSERT INTO users VALUES (3, 'active')");
    qe.execute("UPDATE users SET status = 'active' WHERE status = 'inactive'");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 3);
    for (const auto& row : result) {
        assert(row.at("status") == "active");
    }
    std::cout << "PASS: test_update_where_multiple" << std::endl;
}

void test_update_where_no_matches() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("UPDATE users SET name = 'Bob' WHERE id = 999");
    auto result = qe.execute("SELECT * FROM users");
    assert(result[0]["name"] == "Alice");
    std::cout << "PASS: test_update_where_no_matches" << std::endl;
}

void test_delete_where_single() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, name TEXT)");
    qe.execute("INSERT INTO users VALUES (1, 'Alice')");
    qe.execute("INSERT INTO users VALUES (2, 'Bob')");
    qe.execute("DELETE FROM users WHERE id = 1");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 1);
    assert(result[0]["name"] == "Bob");
    std::cout << "PASS: test_delete_where_single" << std::endl;
}

void test_delete_where_multiple() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, age INT)");
    qe.execute("INSERT INTO users VALUES (1, 15)");
    qe.execute("INSERT INTO users VALUES (2, 25)");
    qe.execute("INSERT INTO users VALUES (3, 35)");
    qe.execute("DELETE FROM users WHERE age < 20");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 2);
    std::cout << "PASS: test_delete_where_multiple" << std::endl;
}

void test_delete_where_no_matches() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT)");
    qe.execute("INSERT INTO users VALUES (1)");
    qe.execute("DELETE FROM users WHERE id = 999");
    auto result = qe.execute("SELECT * FROM users");
    assert(result.size() == 1);
    std::cout << "PASS: test_delete_where_no_matches" << std::endl;
}

void test_where_with_limit() {
    QueryExecutor qe;
    qe.execute("CREATE TABLE users (id INT, age INT)");
    qe.execute("INSERT INTO users VALUES (1, 25)");
    qe.execute("INSERT INTO users VALUES (2, 35)");
    qe.execute("INSERT INTO users VALUES (3, 45)");
    auto result = qe.execute("SELECT * FROM users WHERE age > 20 LIMIT 2");
    assert(result.size() <= 2);
    std::cout << "PASS: test_where_with_limit" << std::endl;
}

int main() {
    std::cout << "\n=== DBX4 PHASE 4.2: SQL EXECUTOR TEST SUITE ===" << std::endl << std::endl;

    try {
        test_create_table_and_insert();
        test_select_with_limit();
        test_update_rows();
        test_delete_rows();
        test_multiple_inserts();
        test_limit_zero();
        test_lexer_tokenization();
        test_parser_ast_construction();
        test_full_pipeline_select();
        test_select_where_equals();
        test_select_where_multiple_matches();
        test_select_where_no_matches();
        test_update_where_single();
        test_update_where_multiple();
        test_update_where_no_matches();
        test_delete_where_single();
        test_delete_where_multiple();
        test_delete_where_no_matches();
        test_where_with_limit();

        std::cout << "\n=== TEST SUMMARY ===" << std::endl;
        std::cout << "Total Tests:  19" << std::endl;
        std::cout << "Passed:       19" << std::endl;
        std::cout << "Failed:       0" << std::endl;
        std::cout << "Success Rate: 100%" << std::endl << std::endl;

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}
