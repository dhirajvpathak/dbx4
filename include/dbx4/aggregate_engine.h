#ifndef DBX4_AGGREGATE_ENGINE_H
#define DBX4_AGGREGATE_ENGINE_H
#include <string>
#include <vector>
#include <map>
#include <limits>
namespace dbx4 {
class AggregateExecutor {
public:
    int execute_count(const std::vector<std::map<std::string, std::string>>& rows) {
        return rows.size();
    }
    double execute_sum(const std::vector<std::map<std::string, std::string>>& rows, const std::string& col) {
        double sum = 0;
        for (const auto& row : rows) {
            if (row.find(col) != row.end()) {
                try { sum += std::stod(row.at(col)); } catch (...) {}
            }
        }
        return sum;
    }
    double execute_avg(const std::vector<std::map<std::string, std::string>>& rows, const std::string& col) {
        if (rows.empty()) return 0;
        return execute_sum(rows, col) / rows.size();
    }
    double execute_min(const std::vector<std::map<std::string, std::string>>& rows, const std::string& col) {
        if (rows.empty()) return 0;
        double min_val = std::numeric_limits<double>::max();
        for (const auto& row : rows) {
            if (row.find(col) != row.end()) {
                try {
                    double val = std::stod(row.at(col));
                    if (val < min_val) min_val = val;
                } catch (...) {}
            }
        }
        return min_val;
    }
    double execute_max(const std::vector<std::map<std::string, std::string>>& rows, const std::string& col) {
        if (rows.empty()) return 0;
        double max_val = std::numeric_limits<double>::lowest();
        for (const auto& row : rows) {
            if (row.find(col) != row.end()) {
                try {
                    double val = std::stod(row.at(col));
                    if (val > max_val) max_val = val;
                } catch (...) {}
            }
        }
        return max_val;
    }
};
}
#endif
