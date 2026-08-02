#include <iostream>
#include <cassert>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

struct Table {
    std::string name;
    std::vector<std::string> columns;
    std::vector<std::map<std::string, std::string>> rows;
};

class QueryExecutor {
private:
    std::map<std::string, Table> tables;
    
public:
    std::vector<std::map<std::string, std::string>> execute(const std::string& sql) {
        std::string trimmed = trim(sql);
        
        if (trimmed.find("CREATE TABLE") == 0) {
            return execute_create_table(trimmed);
        } else if (trimmed.find("INSERT INTO") == 0) {
            return execute_insert(trimmed);
        } else if (trimmed.find("SELECT") == 0) {
            return execute_select(trimmed);
        } else if (trimmed.find("UPDATE") == 0) {
            return execute_update(trimmed);
        } else if (trimmed.find("DELETE FROM") == 0) {
            return execute_delete(trimmed);
        }
        throw std::runtime_error("Unknown statement");
    }
    
private:
    static std::string trim(const std::string& str) {
        size_t start = str.find_first_not_of(" \t\n\r");
        if (start == std::string::npos) return "";
        size_t end = str.find_last_not_of(" \t\n\r");
        return str.substr(start, end - start + 1);
    }
    
    static std::string extract_table_name(const std::string& sql) {
        size_t table_pos = sql.find("TABLE");
        size_t into_pos = sql.find("INTO");
        size_t from_pos = sql.find("FROM");
        size_t update_pos = sql.find("UPDATE");
        
        size_t start_pos = 0;
        if (table_pos != std::string::npos) start_pos = table_pos + 5;
        else if (into_pos != std::string::npos) start_pos = into_pos + 4;
        else if (from_pos != std::string::npos) start_pos = from_pos + 4;
        else if (update_pos != std::string::npos) start_pos = update_pos + 6;
        
        while (start_pos < sql.length() && (sql[start_pos] == ' ' || sql[start_pos] == '\t')) start_pos++;
        
        size_t end_pos = start_pos;
        while (end_pos < sql.length() && sql[end_pos] != ' ' && sql[end_pos] != '\t' && 
               sql[end_pos] != '(' && sql[end_pos] != ')' && sql[end_pos] != ';') {
            end_pos++;
        }
        
        return sql.substr(start_pos, end_pos - start_pos);
    }
    
    std::vector<std::map<std::string, std::string>> execute_create_table(const std::string& sql) {
        std::string table_name = extract_table_name(sql);
        size_t paren_start = sql.find('(');
        size_t paren_end = sql.rfind(')');
        
        if (paren_start == std::string::npos || paren_end == std::string::npos) {
            throw std::runtime_error("Invalid CREATE TABLE syntax");
        }
        
        std::string cols_def = sql.substr(paren_start + 1, paren_end - paren_start - 1);
        std::vector<std::string> columns;
        std::stringstream ss(cols_def);
        std::string col_spec;
        
        while (std::getline(ss, col_spec, ',')) {
            col_spec = trim(col_spec);
            size_t space_pos = col_spec.find(' ');
            if (space_pos != std::string::npos) {
                columns.push_back(col_spec.substr(0, space_pos));
            } else {
                columns.push_back(col_spec);
            }
        }
        
        Table t;
        t.name = table_name;
        t.columns = columns;
        tables[table_name] = t;
        
        return std::vector<std::map<std::string, std::string>>();
    }
    
    std::vector<std::map<std::string, std::string>> execute_insert(const std::string& sql) {
        std::string table_name = extract_table_name(sql);
        
        if (tables.find(table_name) == tables.end()) {
            throw std::runtime_error("Table not found: " + table_name);
        }
        
        Table& t = tables[table_name];
        size_t values_pos = sql.find("VALUES");
        if (values_pos == std::string::npos) {
            throw std::runtime_error("Invalid INSERT: missing VALUES");
        }
        
        size_t paren_start = sql.find('(', values_pos);
        size_t paren_end = sql.rfind(')');
        
        if (paren_start == std::string::npos) {
            throw std::runtime_error("Invalid INSERT: missing parentheses");
        }
        
        std::string values_str = sql.substr(paren_start + 1, paren_end - paren_start - 1);
        std::vector<std::string> values;
        std::stringstream ss(values_str);
        std::string val;
        
        while (std::getline(ss, val, ',')) {
            values.push_back(trim(val));
        }
        
        if (values.size() != t.columns.size()) {
            throw std::runtime_error("Column count mismatch");
        }
        
        std::map<std::string, std::string> row;
        for (size_t i = 0; i < t.columns.size(); i++) {
            std::string value = values[i];
            if ((value[0] == '\'' || value[0] == '"') && value.back() == value[0]) {
                value = value.substr(1, value.length() - 2);
            }
            row[t.columns[i]] = value;
        }
        
        t.rows.push_back(row);
        return std::vector<std::map<std::string, std::string>>();
    }
    
    std::vector<std::map<std::string, std::string>> execute_select(const std::string& sql) {
        std::string table_name = extract_table_name(sql);
        
        if (tables.find(table_name) == tables.end()) {
            throw std::runtime_error("Table not found: " + table_name);
        }
        
        Table& t = tables[table_name];
        std::vector<std::map<std::string, std::string>> result = t.rows;
        
        size_t limit_pos = sql.find("LIMIT");
        if (limit_pos != std::string::npos) {
            size_t num_start = limit_pos + 5;
            while (num_start < sql.length() && (sql[num_start] == ' ' || sql[num_start] == '\t')) {
                num_start++;
            }
            size_t num_end = num_start;
            while (num_end < sql.length() && std::isdigit(sql[num_end])) {
                num_end++;
            }
            
            int limit = std::stoi(sql.substr(num_start, num_end - num_start));
            if ((size_t)limit < result.size()) {
                result.resize(limit);
            }
        }
        
        return result;
    }
    
    std::vector<std::map<std::string, std::string>> execute_update(const std::string& sql) {
        std::string table_name = extract_table_name(sql);
        
        if (tables.find(table_name) == tables.end()) {
            throw std::runtime_error("Table not found: " + table_name);
        }
        
        Table& t = tables[table_name];
        size_t set_pos = sql.find("SET");
        size_t where_pos = sql.find("WHERE");
        
        if (set_pos == std::string::npos) {
            throw std::runtime_error("Invalid UPDATE: missing SET");
        }
        
        size_t set_end = (where_pos != std::string::npos) ? where_pos : sql.length();
        std::string set_clause = sql.substr(set_pos + 3, set_end - set_pos - 3);
        set_clause = trim(set_clause);
        
        size_t eq_pos = set_clause.find('=');
        if (eq_pos == std::string::npos) {
            throw std::runtime_error("Invalid SET clause");
        }
        
        std::string col_name = trim(set_clause.substr(0, eq_pos));
        std::string col_value = trim(set_clause.substr(eq_pos + 1));
        
        if ((col_value[0] == '\'' || col_value[0] == '"') && col_value.back() == col_value[0]) {
            col_value = col_value.substr(1, col_value.length() - 2);
        }
        
        for (auto& row : t.rows) {
            row[col_name] = col_value;
        }
        
        return std::vector<std::map<std::string, std::string>>();
    }
    
    std::vector<std::map<std::string, std::string>> execute_delete(const std::string& sql) {
        std::string table_name = extract_table_name(sql);
        
        if (tables.find(table_name) == tables.end()) {
            throw std::runtime_error("Table not found: " + table_name);
        }
        
        Table& t = tables[table_name];
        t.rows.clear();
        
        return std::vector<std::map<std::string, std::string>>();
    }
};

#define ASSERT_EQ(actual, expected) \
    do { \
        if ((actual) != (expected)) { \
            std::cerr << "ASSERTION FAILED: " << #actual << " != " << #expected << std::endl; \
            std::cerr << "  Expected: " << (expected) << std::endl; \
            std::cerr << "  Actual: " << (actual) << std::endl; \
            exit(1); \
        } \
    } while(0)

int test_count = 0;
int test_passed = 0;
int test_failed = 0;

void test_create_table_and_insert() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        auto result = executor.execute("SELECT * FROM users");
        ASSERT_EQ((int)result.size(), 1);
        ASSERT_EQ(result[0]["id"], "1");
        ASSERT_EQ(result[0]["name"], "Alice");
        test_passed++;
        std::cout << "PASS: test_create_table_and_insert\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_create_table_and_insert - " << e.what() << "\n";
    }
}

void test_select_with_limit() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        executor.execute("INSERT INTO users VALUES (2, 'Bob')");
        executor.execute("INSERT INTO users VALUES (3, 'Charlie')");
        auto result = executor.execute("SELECT * FROM users LIMIT 2");
        ASSERT_EQ((int)result.size(), 2);
        test_passed++;
        std::cout << "PASS: test_select_with_limit\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_select_with_limit - " << e.what() << "\n";
    }
}

void test_update_rows() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        executor.execute("UPDATE users SET name = 'Alicia'");
        auto result = executor.execute("SELECT * FROM users");
        ASSERT_EQ(result[0]["name"], "Alicia");
        test_passed++;
        std::cout << "PASS: test_update_rows\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_update_rows - " << e.what() << "\n";
    }
}

void test_delete_rows() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        executor.execute("INSERT INTO users VALUES (2, 'Bob')");
        executor.execute("DELETE FROM users");
        auto result = executor.execute("SELECT * FROM users");
        ASSERT_EQ((int)result.size(), 0);
        test_passed++;
        std::cout << "PASS: test_delete_rows\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_delete_rows - " << e.what() << "\n";
    }
}

void test_multiple_inserts() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        executor.execute("INSERT INTO users VALUES (2, 'Bob')");
        auto result = executor.execute("SELECT * FROM users");
        ASSERT_EQ((int)result.size(), 2);
        test_passed++;
        std::cout << "PASS: test_multiple_inserts\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_multiple_inserts - " << e.what() << "\n";
    }
}

void test_limit_zero() {
    test_count++;
    try {
        QueryExecutor executor;
        executor.execute("CREATE TABLE users (id INT, name VARCHAR(100))");
        executor.execute("INSERT INTO users VALUES (1, 'Alice')");
        auto result = executor.execute("SELECT * FROM users LIMIT 0");
        ASSERT_EQ((int)result.size(), 0);
        test_passed++;
        std::cout << "PASS: test_limit_zero\n";
    } catch (const std::exception& e) {
        test_failed++;
        std::cout << "FAIL: test_limit_zero - " << e.what() << "\n";
    }
}

int main() {
    std::cout << "\n=== DBX4 PHASE 4.0.2: SQL EXECUTOR TEST SUITE ===\n\n";
    
    test_create_table_and_insert();
    test_select_with_limit();
    test_update_rows();
    test_delete_rows();
    test_multiple_inserts();
    test_limit_zero();

    std::cout << "\n=== TEST SUMMARY ===\n";
    std::cout << "Total Tests:  " << test_count << "\n";
    std::cout << "Passed:       " << test_passed << "\n";
    std::cout << "Failed:       " << test_failed << "\n";
    std::cout << "Success Rate: " << (100.0 * test_passed / test_count) << "%\n\n";

    return (test_failed > 0) ? 1 : 0;
}
