#ifndef DBX4_JOIN_ENGINE_H
#define DBX4_JOIN_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <memory>

namespace dbx4 {

enum class JoinType {
    INNER,
    LEFT_OUTER,
    RIGHT_OUTER,
    FULL_OUTER,
    CROSS
};

struct JoinCondition {
    std::string left_table;
    std::string left_column;
    std::string right_table;
    std::string right_column;
};

class JoinExecutor {
private:
    JoinType join_type_;
    std::string left_table_;
    std::string right_table_;
    JoinCondition condition_;
    
public:
    JoinExecutor(JoinType type, const std::string& left, const std::string& right)
        : join_type_(type), left_table_(left), right_table_(right) {}
    
    bool set_condition(const JoinCondition& cond) {
        condition_ = cond;
        return true;
    }
    
    // Execute INNER JOIN
    std::vector<std::map<std::string, std::string>> execute_inner_join(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        std::vector<std::map<std::string, std::string>> result;
        
        for (const auto& left_row : left_rows) {
            for (const auto& right_row : right_rows) {
                if (left_row.at(condition_.left_column) == 
                    right_row.at(condition_.right_column)) {
                    
                    std::map<std::string, std::string> joined;
                    for (const auto& [k, v] : left_row) {
                        joined[left_table_ + "." + k] = v;
                    }
                    for (const auto& [k, v] : right_row) {
                        joined[right_table_ + "." + k] = v;
                    }
                    result.push_back(joined);
                }
            }
        }
        
        return result;
    }
    
    // Execute LEFT OUTER JOIN
    std::vector<std::map<std::string, std::string>> execute_left_join(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        std::vector<std::map<std::string, std::string>> result;
        std::vector<bool> right_matched(right_rows.size(), false);
        
        for (const auto& left_row : left_rows) {
            bool found = false;
            
            for (size_t i = 0; i < right_rows.size(); ++i) {
                if (left_row.at(condition_.left_column) == 
                    right_rows[i].at(condition_.right_column)) {
                    
                    std::map<std::string, std::string> joined;
                    for (const auto& [k, v] : left_row) {
                        joined[left_table_ + "." + k] = v;
                    }
                    for (const auto& [k, v] : right_rows[i]) {
                        joined[right_table_ + "." + k] = v;
                    }
                    result.push_back(joined);
                    right_matched[i] = true;
                    found = true;
                }
            }
            
            if (!found) {
                std::map<std::string, std::string> joined;
                for (const auto& [k, v] : left_row) {
                    joined[left_table_ + "." + k] = v;
                }
                result.push_back(joined);
            }
        }
        
        return result;
    }
    
    // Execute RIGHT OUTER JOIN
    std::vector<std::map<std::string, std::string>> execute_right_join(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        return execute_left_join(right_rows, left_rows);
    }
    
    // Execute FULL OUTER JOIN
    std::vector<std::map<std::string, std::string>> execute_full_join(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        auto result = execute_left_join(left_rows, right_rows);
        
        for (const auto& right_row : right_rows) {
            bool found = false;
            for (const auto& left_row : left_rows) {
                if (left_row.at(condition_.left_column) == 
                    right_row.at(condition_.right_column)) {
                    found = true;
                    break;
                }
            }
            if (!found) {
                std::map<std::string, std::string> joined;
                for (const auto& [k, v] : right_row) {
                    joined[right_table_ + "." + k] = v;
                }
                result.push_back(joined);
            }
        }
        
        return result;
    }
    
    // Execute CROSS JOIN
    std::vector<std::map<std::string, std::string>> execute_cross_join(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        std::vector<std::map<std::string, std::string>> result;
        
        for (const auto& left_row : left_rows) {
            for (const auto& right_row : right_rows) {
                std::map<std::string, std::string> joined;
                for (const auto& [k, v] : left_row) {
                    joined[left_table_ + "." + k] = v;
                }
                for (const auto& [k, v] : right_row) {
                    joined[right_table_ + "." + k] = v;
                }
                result.push_back(joined);
            }
        }
        
        return result;
    }
    
    std::vector<std::map<std::string, std::string>> execute(
        const std::vector<std::map<std::string, std::string>>& left_rows,
        const std::vector<std::map<std::string, std::string>>& right_rows) {
        
        switch (join_type_) {
            case JoinType::INNER:
                return execute_inner_join(left_rows, right_rows);
            case JoinType::LEFT_OUTER:
                return execute_left_join(left_rows, right_rows);
            case JoinType::RIGHT_OUTER:
                return execute_right_join(left_rows, right_rows);
            case JoinType::FULL_OUTER:
                return execute_full_join(left_rows, right_rows);
            case JoinType::CROSS:
                return execute_cross_join(left_rows, right_rows);
            default:
                return {};
        }
    }
};

}

#endif
