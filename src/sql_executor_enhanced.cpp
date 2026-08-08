#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <sstream>

namespace dbx4 {

struct Row {
    std::map<std::string, std::string> columns;
};

class SQLExecutor {
public:
    // Parse AND clause - not partial interpretation
    static bool evaluate_and_condition(
        const std::string& condition,
        const Row& row
    ) {
        // Split by AND
        std::vector<std::string> parts;
        std::stringstream ss(condition);
        std::string part;
        while (std::getline(ss, part, ' ')) {
            if (!part.empty()) parts.push_back(part);
        }
        
        // Check if AND exists - if so, it's multi-part
        bool has_and = false;
        for (const auto& p : parts) {
            if (p == "AND") {
                has_and = true;
                break;
            }
        }
        
        if (!has_and) {
            // Single condition
            return evaluate_single_condition(condition, row);
        }
        
        // Multiple conditions with AND
        std::string current = "";
        bool all_true = true;
        
        for (size_t i = 0; i < parts.size(); i++) {
            if (parts[i] == "AND") {
                if (!current.empty()) {
                    if (!evaluate_single_condition(current, row)) {
                        all_true = false;
                        break;
                    }
                    current = "";
                }
            } else {
                if (!current.empty()) current += " ";
                current += parts[i];
            }
        }
        
        if (!current.empty() && all_true) {
            all_true = evaluate_single_condition(current, row);
        }
        
        return all_true;
    }
    
    static bool evaluate_single_condition(
        const std::string& condition,
        const Row& row
    ) {
        // Parse: column=value or column>value, etc.
        size_t eq_pos = condition.find('=');
        if (eq_pos == std::string::npos) return false;
        
        std::string col = condition.substr(0, eq_pos);
        std::string val = condition.substr(eq_pos + 1);
        
        // Trim spaces
        while (!col.empty() && col.back() == ' ') col.pop_back();
        while (!val.empty() && val.front() == ' ') val.erase(0, 1);
        
        auto it = row.columns.find(col);
        return it != row.columns.end() && it->second == val;
    }
    
    static std::vector<Row> execute_select(
        const std::vector<Row>& rows,
        const std::vector<std::string>& projection,
        const std::string& order_by,
        int limit,
        const std::string& where_clause
    ) {
        std::vector<Row> result;
        
        // Apply WHERE clause
        for (const auto& row : rows) {
            if (where_clause.empty() || evaluate_and_condition(where_clause, row)) {
                result.push_back(row);
            }
        }
        
        // Apply projection (SELECT specific columns)
        if (!projection.empty() && projection[0] != "*") {
            for (auto& row : result) {
                std::map<std::string, std::string> projected;
                for (const auto& col : projection) {
                    auto it = row.columns.find(col);
                    if (it != row.columns.end()) {
                        projected[col] = it->second;
                    }
                }
                row.columns = projected;
            }
        }
        
        // Apply ORDER BY
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
        
        // Apply LIMIT
        if (limit > 0 && (int)result.size() > limit) {
            result.resize(limit);
        }
        
        return result;
    }
};

}
