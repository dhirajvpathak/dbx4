#ifndef DBX4_AGGREGATE_ENGINE_H
#define DBX4_AGGREGATE_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <cmath>

namespace dbx4 {

enum class AggregateFunction {
    COUNT,
    SUM,
    AVG,
    MIN,
    MAX
};

struct AggregateResult {
    std::string group_key;
    double count_val = 0;
    double sum_val = 0;
    double avg_val = 0;
    double min_val = 0;
    double max_val = 0;
};

class AggregateExecutor {
private:
    std::vector<AggregateFunction> functions_;
    std::vector<std::string> columns_;
    std::string group_by_column_;
    std::string having_clause_;
    
public:
    AggregateExecutor() {}
    
    void add_function(AggregateFunction func, const std::string& col) {
        functions_.push_back(func);
        columns_.push_back(col);
    }
    
    void set_group_by(const std::string& col) {
        group_by_column_ = col;
    }
    
    void set_having(const std::string& clause) {
        having_clause_ = clause;
    }
    
    // COUNT(*)
    int execute_count(const std::vector<std::map<std::string, std::string>>& rows) {
        return rows.size();
    }
    
    // SUM(column)
    double execute_sum(
        const std::vector<std::map<std::string, std::string>>& rows,
        const std::string& column) {
        
        double sum = 0;
        for (const auto& row : rows) {
            if (row.find(column) != row.end()) {
                try {
                    sum += std::stod(row.at(column));
                } catch (...) {}
            }
        }
        return sum;
    }
    
    // AVG(column)
    double execute_avg(
        const std::vector<std::map<std::string, std::string>>& rows,
        const std::string& column) {
        
        if (rows.empty()) return 0;
        double sum = execute_sum(rows, column);
        return sum / rows.size();
    }
    
    // MIN(column)
    double execute_min(
        const std::vector<std::map<std::string, std::string>>& rows,
        const std::string& column) {
        
        if (rows.empty()) return 0;
        double min_val = std::numeric_limits<double>::max();
        
        for (const auto& row : rows) {
            if (row.find(column) != row.end()) {
                try {
                    double val = std::stod(row.at(column));
                    if (val < min_val) min_val = val;
                } catch (...) {}
            }
        }
        
        return min_val == std::numeric_limits<double>::max() ? 0 : min_val;
    }
    
    // MAX(column)
    double execute_max(
        const std::vector<std::map<std::string, std::string>>& rows,
        const std::string& column) {
        
        if (rows.empty()) return 0;
        double max_val = std::numeric_limits<double>::lowest();
        
        for (const auto& row : rows) {
            if (row.find(column) != row.end()) {
                try {
                    double val = std::stod(row.at(column));
                    if (val > max_val) max_val = val;
                } catch (...) {}
            }
        }
        
        return max_val == std::numeric_limits<double>::lowest() ? 0 : max_val;
    }
    
    // GROUP BY execution
    std::map<std::string, std::vector<std::map<std::string, std::string>>> 
    execute_group_by(const std::vector<std::map<std::string, std::string>>& rows) {
        
        std::map<std::string, std::vector<std::map<std::string, std::string>>> groups;
        
        for (const auto& row : rows) {
            std::string key;
            if (row.find(group_by_column_) != row.end()) {
                key = row.at(group_by_column_);
            } else {
                key = "NULL";
            }
            groups[key].push_back(row);
        }
        
        return groups;
    }
    
    // Aggregate with GROUP BY
    std::vector<AggregateResult> execute_with_group_by(
        const std::vector<std::map<std::string, std::string>>& rows) {
        
        auto groups = execute_group_by(rows);
        std::vector<AggregateResult> results;
        
        for (const auto& [key, group_rows] : groups) {
            AggregateResult res;
            res.group_key = key;
            res.count_val = execute_count(group_rows);
            
            if (!columns_.empty()) {
                res.sum_val = execute_sum(group_rows, columns_[0]);
                res.avg_val = execute_avg(group_rows, columns_[0]);
                res.min_val = execute_min(group_rows, columns_[0]);
                res.max_val = execute_max(group_rows, columns_[0]);
            }
            
            results.push_back(res);
        }
        
        return results;
    }
};

}

#endif
