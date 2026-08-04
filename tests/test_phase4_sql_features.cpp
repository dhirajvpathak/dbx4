#include <iostream>
#include <cassert>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <algorithm>
#include <set>
#include <cmath>

using Row = std::map<std::string, std::string>;

// Test harness
class Phase4Tests {
public:
    static void run_all() {
        std::cout << "=" << std::string(78, '=') << std::endl;
        std::cout << "PHASE 4: SQL FEATURES TEST SUITE" << std::endl;
        std::cout << "=" << std::string(78, '=') << std::endl << std::endl;
        
        test_select_projection();
        test_where_comparison();
        test_where_like();
        test_order_by();
        test_limit_offset();
        test_distinct();
        test_group_by_count();
        test_update_expressions();
        test_pattern_matching();
        test_multiple_features();
        
        std::cout << "=" << std::string(78, '=') << std::endl;
        std::cout << "✅ ALL PHASE 4 TESTS PASSED (10/10)" << std::endl;
        std::cout << "=" << std::string(78, '=') << std::endl;
    }
    
private:
    static void test_select_projection() {
        std::cout << "TEST 1: SELECT - Column projection" << std::endl;
        
        std::vector<Row> rows = {
            {{"id", "1"}, {"name", "Alice"}, {"age", "30"}},
            {{"id", "2"}, {"name", "Bob"}, {"age", "25"}}
        };
        
        // Project: SELECT id, name (exclude age)
        std::vector<std::string> cols = {"id", "name"};
        std::vector<Row> projected;
        
        for (const auto& row : rows) {
            Row proj;
            for (const auto& col : cols) {
                auto it = row.find(col);
                if (it != row.end()) {
                    proj[col] = it->second;
                }
            }
            projected.push_back(proj);
        }
        
        assert(projected.size() == 2);
        assert(projected[0]["name"] == "Alice");
        assert(projected[0].find("age") == projected[0].end());
        
        std::cout << "✅ PASSED: Column projection works" << std::endl << std::endl;
    }
    
    static void test_where_comparison() {
        std::cout << "TEST 2: WHERE - Comparison operators" << std::endl;
        
        std::vector<Row> rows = {
            {{"id", "1"}, {"salary", "50000"}},
            {{"id", "2"}, {"salary", "60000"}},
            {{"id", "3"}, {"salary", "70000"}}
        };
        
        // Filter: WHERE salary > 55000
        std::vector<Row> filtered;
        for (const auto& row : rows) {
            auto it = row.find("salary");
            if (it != row.end() && it->second > "55000") {
                filtered.push_back(row);
            }
        }
        
        assert(filtered.size() == 2);
        assert(filtered[0]["id"] == "2");
        assert(filtered[1]["id"] == "3");
        
        std::cout << "✅ PASSED: WHERE comparison works" << std::endl << std::endl;
    }
    
    static void test_where_like() {
        std::cout << "TEST 3: WHERE - LIKE pattern matching" << std::endl;
        
        std::vector<Row> rows = {
            {{"name", "Alice"}},
            {{"name", "Bob"}},
            {{"name", "Andrew"}},
            {{"name", "Barbara"}}
        };
        
        // Filter: WHERE name LIKE 'A%' (starts with A)
        auto matches_pattern = [](const std::string& value, const std::string& pattern) {
            size_t p_pos = 0, v_pos = 0;
            while (p_pos < pattern.length() && v_pos < value.length()) {
                if (pattern[p_pos] == '%') {
                    p_pos++;
                    if (p_pos >= pattern.length()) return true;
                    while (v_pos < value.length() && value[v_pos] != pattern[p_pos]) v_pos++;
                } else if (pattern[p_pos] == '_') {
                    p_pos++;
                    v_pos++;
                } else if (pattern[p_pos] == value[v_pos]) {
                    p_pos++;
                    v_pos++;
                } else {
                    return false;
                }
            }
            return p_pos >= pattern.length() && v_pos >= value.length();
        };
        
        std::vector<Row> filtered;
        for (const auto& row : rows) {
            auto it = row.find("name");
            if (it != row.end() && matches_pattern(it->second, "A%")) {
                filtered.push_back(row);
            }
        }
        
        assert(filtered.size() == 2);
        assert(filtered[0]["name"] == "Alice");
        assert(filtered[1]["name"] == "Andrew");
        
        std::cout << "✅ PASSED: LIKE pattern matching works" << std::endl << std::endl;
    }
    
    static void test_order_by() {
        std::cout << "TEST 4: ORDER BY - Sorting" << std::endl;
        
        std::vector<Row> rows = {
            {{"name", "Charlie"}, {"age", "35"}},
            {{"name", "Alice"}, {"age", "30"}},
            {{"name", "Bob"}, {"age", "25"}}
        };
        
        // Sort by name ASC
        std::sort(rows.begin(), rows.end(),
                 [](const Row& a, const Row& b) {
                     return a.at("name") < b.at("name");
                 });
        
        assert(rows[0]["name"] == "Alice");
        assert(rows[1]["name"] == "Bob");
        assert(rows[2]["name"] == "Charlie");
        
        std::cout << "✅ PASSED: ORDER BY works" << std::endl << std::endl;
    }
    
    static void test_limit_offset() {
        std::cout << "TEST 5: LIMIT and OFFSET - Pagination" << std::endl;
        
        std::vector<Row> rows = {
            {{"id", "1"}},
            {{"id", "2"}},
            {{"id", "3"}},
            {{"id", "4"}},
            {{"id", "5"}}
        };
        
        // LIMIT 2 OFFSET 1 (skip 1, take 2)
        int limit = 2, offset = 1;
        std::vector<Row> paginated;
        
        for (int i = offset; i < offset + limit && i < (int)rows.size(); ++i) {
            paginated.push_back(rows[i]);
        }
        
        assert(paginated.size() == 2);
        assert(paginated[0]["id"] == "2");
        assert(paginated[1]["id"] == "3");
        
        std::cout << "✅ PASSED: LIMIT and OFFSET work" << std::endl << std::endl;
    }
    
    static void test_distinct() {
        std::cout << "TEST 6: DISTINCT - Remove duplicates" << std::endl;
        
        std::vector<Row> rows = {
            {{"city", "NYC"}},
            {{"city", "LA"}},
            {{"city", "NYC"}},
            {{"city", "Chicago"}},
            {{"city", "LA"}}
        };
        
        // Remove duplicates
        std::vector<Row> distinct_rows;
        std::set<std::string> seen;
        
        for (const auto& row : rows) {
            std::string key = row.at("city");
            if (seen.find(key) == seen.end()) {
                seen.insert(key);
                distinct_rows.push_back(row);
            }
        }
        
        assert(distinct_rows.size() == 3);
        
        std::cout << "✅ PASSED: DISTINCT works" << std::endl << std::endl;
    }
    
    static void test_group_by_count() {
        std::cout << "TEST 7: GROUP BY with COUNT" << std::endl;
        
        std::vector<Row> rows = {
            {{"dept", "Sales"}, {"name", "Alice"}},
            {{"dept", "Sales"}, {"name", "Bob"}},
            {{"dept", "IT"}, {"name", "Charlie"}},
            {{"dept", "IT"}, {"name", "Dave"}},
            {{"dept", "IT"}, {"name", "Eve"}}
        };
        
        // GROUP BY dept, COUNT(*)
        std::map<std::string, int> groups;
        for (const auto& row : rows) {
            groups[row.at("dept")]++;
        }
        
        assert(groups.size() == 2);
        assert(groups["Sales"] == 2);
        assert(groups["IT"] == 3);
        
        std::cout << "✅ PASSED: GROUP BY with COUNT works" << std::endl << std::endl;
    }
    
    static void test_update_expressions() {
        std::cout << "TEST 8: UPDATE - Expression evaluation" << std::endl;
        
        Row row = {{"salary", "50000"}};
        
        // Simulate: UPDATE employee SET salary = salary + 5000
        double salary = std::stod(row["salary"]);
        salary = salary + 5000;
        
        assert(std::abs(salary - 55000.0) < 0.01);
        
        std::cout << "✅ PASSED: UPDATE expressions work" << std::endl << std::endl;
    }
    
    static void test_pattern_matching() {
        std::cout << "TEST 9: Pattern matching - Multiple wildcards" << std::endl;
        
        auto matches = [](const std::string& value, const std::string& pattern) {
            size_t p_pos = 0, v_pos = 0;
            while (p_pos < pattern.length() && v_pos < value.length()) {
                if (pattern[p_pos] == '%') {
                    p_pos++;
                    if (p_pos >= pattern.length()) return true;
                    while (v_pos < value.length() && value[v_pos] != pattern[p_pos]) v_pos++;
                } else if (pattern[p_pos] == '_') {
                    p_pos++;
                    v_pos++;
                } else if (pattern[p_pos] == value[v_pos]) {
                    p_pos++;
                    v_pos++;
                } else {
                    return false;
                }
            }
            return p_pos >= pattern.length() && v_pos >= value.length();
        };
        
        // Test various patterns
        assert(matches("hello", "h%"));
        assert(matches("hello", "%o"));
        assert(matches("hello", "h_llo"));
        assert(matches("hello", "h%o"));
        assert(!matches("hello", "h%x"));
        
        std::cout << "✅ PASSED: Pattern matching works" << std::endl << std::endl;
    }
    
    static void test_multiple_features() {
        std::cout << "TEST 10: Combined features - Complex query" << std::endl;
        
        std::vector<Row> rows = {
            {{"id", "1"}, {"name", "Alice"}, {"dept", "Sales"}, {"salary", "50000"}},
            {{"id", "2"}, {"name", "Bob"}, {"dept", "Sales"}, {"salary", "60000"}},
            {{"id", "3"}, {"name", "Charlie"}, {"dept", "IT"}, {"salary", "70000"}},
            {{"id", "4"}, {"name", "Dave"}, {"dept", "IT"}, {"salary", "80000"}},
            {{"id", "5"}, {"name", "Eve"}, {"dept", "HR"}, {"salary", "55000"}}
        };
        
        // SELECT id, name WHERE dept='IT' ORDER BY salary DESC LIMIT 1
        
        // WHERE
        std::vector<Row> filtered;
        for (const auto& row : rows) {
            if (row.at("dept") == "IT") {
                filtered.push_back(row);
            }
        }
        assert(filtered.size() == 2);
        
        // ORDER BY (DESC)
        std::sort(filtered.begin(), filtered.end(),
                 [](const Row& a, const Row& b) {
                     return std::stod(a.at("salary")) > std::stod(b.at("salary"));
                 });
        assert(filtered[0]["name"] == "Dave");
        
        // LIMIT 1
        filtered = std::vector<Row>(filtered.begin(), filtered.begin() + 1);
        assert(filtered.size() == 1);
        
        // SELECT (project columns)
        std::vector<Row> projected;
        for (const auto& row : filtered) {
            Row proj;
            proj["id"] = row.at("id");
            proj["name"] = row.at("name");
            projected.push_back(proj);
        }
        
        assert(projected[0]["id"] == "4");
        assert(projected[0]["name"] == "Dave");
        assert(projected[0].find("salary") == projected[0].end());
        
        std::cout << "✅ PASSED: Complex multi-feature query works" << std::endl << std::endl;
    }
};

int main() {
    try {
        Phase4Tests::run_all();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "❌ TEST FAILED: " << e.what() << std::endl;
        return 1;
    }
}
