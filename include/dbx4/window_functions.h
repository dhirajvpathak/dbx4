#ifndef DBX4_WINDOW_FUNCTIONS_H
#define DBX4_WINDOW_FUNCTIONS_H

#include <string>
#include <vector>
#include <map>
#include <algorithm>

namespace dbx4 {

struct WindowSpec {
    std::string partition_by;
    std::string order_by;
    std::string frame;
};

struct WindowResult {
    int row_number = 0;
    int rank = 0;
    int dense_rank = 0;
    std::string first_value;
    std::string last_value;
    std::string lag_value;
    std::string lead_value;
};

class WindowFunctionExecutor {
private:
    std::vector<std::map<std::string, std::string>> rows_;
    WindowSpec window_spec_;
    
public:
    WindowFunctionExecutor() {}
    
    void set_window_spec(const WindowSpec& spec) {
        window_spec_ = spec;
    }
    
    // ROW_NUMBER() - assigns unique number to each row
    std::vector<int> execute_row_number() {
        std::vector<int> result;
        for (size_t i = 0; i < rows_.size(); ++i) {
            result.push_back(i + 1);
        }
        return result;
    }
    
    // RANK() - same rank for ties, skips next rank
    std::vector<int> execute_rank(const std::vector<int>& values) {
        std::vector<int> result;
        int rank = 1;
        int count = 0;
        int last_value = -1;
        
        for (int val : values) {
            count++;
            if (val != last_value) {
                rank = count;
                last_value = val;
            }
            result.push_back(rank);
        }
        return result;
    }
    
    // DENSE_RANK() - same rank for ties, no gap in ranks
    std::vector<int> execute_dense_rank(const std::vector<int>& values) {
        std::vector<int> result;
        int rank = 1;
        int last_value = -1;
        
        for (int val : values) {
            if (val != last_value) {
                rank++;
                last_value = val;
            }
            result.push_back(rank);
        }
        return result;
    }
    
    // LAG() - access previous row
    std::vector<std::string> execute_lag(const std::vector<std::string>& values, int offset = 1) {
        std::vector<std::string> result;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i >= offset) {
                result.push_back(values[i - offset]);
            } else {
                result.push_back("");  // NULL
            }
        }
        return result;
    }
    
    // LEAD() - access next row
    std::vector<std::string> execute_lead(const std::vector<std::string>& values, int offset = 1) {
        std::vector<std::string> result;
        for (size_t i = 0; i < values.size(); ++i) {
            if (i + offset < values.size()) {
                result.push_back(values[i + offset]);
            } else {
                result.push_back("");  // NULL
            }
        }
        return result;
    }
    
    // FIRST_VALUE() - first value in window
    std::string execute_first_value(const std::vector<std::string>& values) {
        if (!values.empty()) {
            return values[0];
        }
        return "";
    }
    
    // LAST_VALUE() - last value in window
    std::string execute_last_value(const std::vector<std::string>& values) {
        if (!values.empty()) {
            return values[values.size() - 1];
        }
        return "";
    }
    
    // PARTITION BY support
    std::vector<std::vector<std::map<std::string, std::string>>> partition_rows(
        const std::string& partition_key) {
        
        std::map<std::string, std::vector<std::map<std::string, std::string>>> partitions;
        
        for (const auto& row : rows_) {
            if (row.find(partition_key) != row.end()) {
                partitions[row.at(partition_key)].push_back(row);
            }
        }
        
        std::vector<std::vector<std::map<std::string, std::string>>> result;
        for (auto& [key, partition] : partitions) {
            result.push_back(partition);
        }
        return result;
    }
};

}

#endif
