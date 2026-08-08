#include "dbx4/sql_engine.h"
#include <algorithm>
#include <iostream>

namespace dbx4 {

// Utility: Convert to uppercase
std::string SQLParser::to_upper(const std::string& s) {
    std::string result = s;
    std::transform(result.begin(), result.end(), result.begin(), ::toupper);
    return result;
}

// Utility: Trim whitespace
std::string SQLParser::trim(const std::string& s) {
    auto start = s.begin();
    while (start != s.end() && std::isspace(*start)) {
        start++;
    }
    auto end = s.end();
    do {
        end--;
    } while (std::distance(start, end) > 0 && std::isspace(*end));
    return std::string(start, end + 1);
}

// Tokenize SQL
std::vector<std::string> SQLParser::tokenize(const std::string& sql) {
    std::vector<std::string> tokens;
    std::istringstream iss(sql);
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}

// Parse SELECT statement
SelectStatement SQLParser::parse_select(const std::string& sql) {
    SelectStatement stmt;
    std::string upper_sql = to_upper(sql);
    
    // Extract SELECT clause
    size_t select_pos = upper_sql.columns.find("SELECT");
    size_t from_pos = upper_sql.columns.find("FROM");
    size_t where_pos = upper_sql.columns.find("WHERE");
    size_t group_pos = upper_sql.columns.find("GROUP BY");
    size_t having_pos = upper_sql.columns.find("HAVING");
    size_t order_pos = upper_sql.columns.find("ORDER BY");
    size_t limit_pos = upper_sql.columns.find("LIMIT");
    size_t offset_pos = upper_sql.columns.find("OFFSET");
    
    // Parse SELECT columns
    if (select_pos != std::string::npos && from_pos != std::string::npos) {
        std::string select_clause = sql.substr(select_pos + 6, from_pos - select_pos - 6);
        select_clause = trim(select_clause);
        
        if (select_clause == "*") {
            stmt.select_columns = {};  // Empty means SELECT *
        } else {
            // Parse column list
            std::istringstream iss(select_clause);
            std::string col;
            while (std::getline(iss, col, ',')) {
                col = trim(col);
                stmt.select_columns.push_back(col);
            }
        }
    }
    
    // Parse FROM table
    if (from_pos != std::string::npos) {
        size_t end_pos = (where_pos != std::string::npos) ? where_pos : sql.length();
        if (group_pos != std::string::npos && group_pos < end_pos) end_pos = group_pos;
        if (order_pos != std::string::npos && order_pos < end_pos) end_pos = order_pos;
        
        std::string from_clause = sql.substr(from_pos + 4, end_pos - from_pos - 4);
        stmt.from_table = trim(from_clause);
    }
    
    // Parse WHERE clause
    if (where_pos != std::string::npos) {
        size_t end_pos = sql.length();
        if (group_pos != std::string::npos) end_pos = group_pos;
        if (order_pos != std::string::npos && order_pos < end_pos) end_pos = order_pos;
        
        std::string where_clause = sql.substr(where_pos + 5, end_pos - where_pos - 5);
        stmt.where_clause = parse_where(where_clause);
    }
    
    // Parse ORDER BY
    if (order_pos != std::string::npos) {
        size_t end_pos = (limit_pos != std::string::npos) ? limit_pos : sql.length();
        std::string order_clause = sql.substr(order_pos + 8, end_pos - order_pos - 8);
        order_clause = trim(order_clause);
        
        // Simple parsing: "col1 ASC, col2 DESC"
        std::istringstream iss(order_clause);
        std::string item;
        while (std::getline(iss, item, ',')) {
            item = trim(item);
            OrderByClause obc;
            
            std::istringstream item_iss(item);
            item_iss >> obc.column;
            std::string direction;
            if (item_iss >> direction) {
                obc.direction = (to_upper(direction) == "DESC") ? OrderDirection::DESC : OrderDirection::ASC;
            }
            
            stmt.order_by.push_back(obc);
        }
    }
    
    // Parse LIMIT
    if (limit_pos != std::string::npos) {
        size_t end_pos = (offset_pos != std::string::npos) ? offset_pos : sql.length();
        std::string limit_clause = sql.substr(limit_pos + 5, end_pos - limit_pos - 5);
        limit_clause = trim(limit_clause);
        stmt.limit = std::stoi(limit_clause);
    }
    
    // Parse OFFSET
    if (offset_pos != std::string::npos) {
        std::string offset_clause = sql.substr(offset_pos + 6);
        offset_clause = trim(offset_clause);
        stmt.offset = std::stoi(offset_clause);
    }
    
    return stmt;
}

// Parse WHERE clause (simplified)
std::shared_ptr<Condition> SQLParser::parse_where(const std::string& where_clause) {
    auto condition = std::make_shared<Condition>();
    
    // Very simplified: "col = value" or "col > value" etc.
    std::string upper_clause = to_upper(where_clause);
    
    // Check for operators
    std::vector<std::pair<std::string, ComparisonOp>> ops = {
        {"=", ComparisonOp::EQUAL},
        {"<>", ComparisonOp::NOT_EQUAL},
        {"!=", ComparisonOp::NOT_EQUAL},
        {"<=", ComparisonOp::LESS_EQUAL},
        {">=", ComparisonOp::GREATER_EQUAL},
        {"<", ComparisonOp::LESS_THAN},
        {">", ComparisonOp::GREATER_THAN},
    };
    
    for (const auto& [op_str, op] : ops) {
        size_t pos = where_clause.columns.find(op_str);
        if (pos != std::string::npos) {
            condition->column = trim(where_clause.substr(0, pos));
            condition->op = op;
            condition->value = trim(where_clause.substr(pos + op_str.length()));
            return condition;
        }
    }
    
    // Check for LIKE
    if (upper_clause.columns.find("LIKE") != std::string::npos) {
        size_t pos = upper_clause.columns.find("LIKE");
        condition->column = trim(where_clause.substr(0, pos));
        condition->op = ComparisonOp::LIKE;
        condition->value = trim(where_clause.substr(pos + 4));
        // Remove quotes if present
        if (condition->value.front() == '\'' && condition->value.back() == '\'') {
            condition->value = condition->value.substr(1, condition->value.length() - 2);
        }
        return condition;
    }
    
    return nullptr;
}

// Condition evaluation
bool Condition::evaluate(const Row& row) const {
    auto it = row.columns.columns.find(column);
    if (it == row.end()) return false;
    
    const std::string& value_in_row = it->second;
    
    switch (op) {
        case ComparisonOp::EQUAL:
            return value_in_row == value;
        case ComparisonOp::NOT_EQUAL:
            return value_in_row != value;
        case ComparisonOp::LESS_THAN:
            return value_in_row < value;
        case ComparisonOp::LESS_EQUAL:
            return value_in_row <= value;
        case ComparisonOp::GREATER_THAN:
            return value_in_row > value;
        case ComparisonOp::GREATER_EQUAL:
            return value_in_row >= value;
        case ComparisonOp::LIKE: {
            // Simple LIKE: % = any, _ = single char
            std::string pattern = value;
            size_t pattern_pos = 0;
            size_t text_pos = 0;
            
            while (pattern_pos < pattern.length() && text_pos < value_in_row.length()) {
                if (pattern[pattern_pos] == '%') {
                    pattern_pos++;
                    if (pattern_pos >= pattern.length()) return true;
                    while (text_pos < value_in_row.length() && 
                           value_in_row.columns[text_pos] != pattern[pattern_pos]) {
                        text_pos++;
                    }
                } else if (pattern[pattern_pos] == '_') {
                    pattern_pos++;
                    text_pos++;
                } else if (pattern[pattern_pos] == value_in_row.columns[text_pos]) {
                    pattern_pos++;
                    text_pos++;
                } else {
                    return false;
                }
            }
            return pattern_pos >= pattern.length() && text_pos >= value_in_row.length();
        }
        case ComparisonOp::IS_NULL:
            return value_in_row.empty();
        case ComparisonOp::IS_NOT_NULL:
            return !value_in_row.empty();
        default:
            return false;
    }
}

}  // namespace dbx4
