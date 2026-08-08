#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cctype>
#include <set>

namespace dbx4 {

struct Row {
    std::map<std::string, std::string> columns;
};

class SQLExecutor {
private:
    // Parse column list from projection
    static std::vector<std::string> parse_columns(const std::string& proj) {
        std::vector<std::string> cols;
        if (proj == "*") return cols;  // Empty means all columns
        
        std::string current;
        for (char c : proj) {
            if (c == ',') {
                if (!current.empty()) {
                    // Trim
                    while (!current.empty() && std::isspace(current.front())) current.erase(0, 1);
                    while (!current.empty() && std::isspace(current.back())) current.pop_back();
                    cols.push_back(current);
                    current.clear();
                }
            } else {
                current += c;
            }
        }
        if (!current.empty()) {
            while (!current.empty() && std::isspace(current.front())) current.erase(0, 1);
            while (!current.empty() && std::isspace(current.back())) current.pop_back();
            cols.push_back(current);
        }
        return cols;
    }
    
    // Evaluate WHERE clause with proper AND/OR support
    static bool evaluate_where(const std::string& where_clause, const Row& row) {
        if (where_clause.empty()) return true;
        
        // Check for AND
        size_t and_pos = where_clause.find(" AND ");
        if (and_pos != std::string::npos) {
            std::string left = where_clause.substr(0, and_pos);
            std::string right = where_clause.substr(and_pos + 5);
            return evaluate_where(left, row) && evaluate_where(right, row);
        }
        
        // Check for OR
        size_t or_pos = where_clause.find(" OR ");
        if (or_pos != std::string::npos) {
            std::string left = where_clause.substr(0, or_pos);
            std::string right = where_clause.substr(or_pos + 4);
            return evaluate_where(left, row) || evaluate_where(right, row);
        }
        
        // Check for NOT
        if (where_clause.find("NOT ") == 0) {
            return !evaluate_where(where_clause.substr(4), row);
        }
        
        // Single condition: column=value
        size_t eq_pos = where_clause.find('=');
        if (eq_pos == std::string::npos) {
            throw std::runtime_error("Unsupported WHERE syntax: " + where_clause);
        }
        
        std::string col = where_clause.substr(0, eq_pos);
        std::string val = where_clause.substr(eq_pos + 1);
        
        // Trim
        while (!col.empty() && std::isspace(col.front())) col.erase(0, 1);
        while (!col.empty() && std::isspace(col.back())) col.pop_back();
        while (!val.empty() && std::isspace(val.front())) val.erase(0, 1);
        while (!val.empty() && std::isspace(val.back())) val.pop_back();
        
        auto it = row.columns.find(col);
        if (it == row.columns.end()) return false;
        return it->second == val;
    }
    
public:
    static std::vector<Row> execute_select(
        const std::vector<Row>& rows,
        const std::string& projection,
        const std::string& order_by,
        int limit,
        const std::string& where_clause
    ) {
        std::vector<Row> result;
        
        // Step 1: Apply WHERE
        for (const auto& row : rows) {
            try {
                if (evaluate_where(where_clause, row)) {
                    result.push_back(row);
                }
            } catch (const std::exception& e) {
                // Unsupported syntax throws
                throw;
            }
        }
        
        // Step 2: Apply ORDER BY
        if (!order_by.empty() && order_by != "*") {
            std::sort(result.begin(), result.end(),
                [&order_by](const Row& a, const Row& b) {
                    auto it_a = a.columns.find(order_by);
                    auto it_b = b.columns.find(order_by);
                    if (it_a == a.columns.end()) return true;
                    if (it_b == b.columns.end()) return false;
                    return it_a->second < it_b->second;
                }
            );
        }
        
        // Step 3: Apply LIMIT
        if (limit > 0 && (int)result.size() > limit) {
            result.resize(limit);
        }
        
        // Step 4: Apply projection
        auto cols = parse_columns(projection);
        if (!cols.empty()) {
            for (auto& row : result) {
                std::map<std::string, std::string> projected;
                for (const auto& col : cols) {
                    auto it = row.columns.find(col);
                    if (it != row.columns.end()) {
                        projected[col] = it->second;
                    }
                }
                row.columns = projected;
            }
        }
        
        return result;
    }
};

}
