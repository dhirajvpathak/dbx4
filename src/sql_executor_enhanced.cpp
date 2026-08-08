#include <string>
#include <vector>
#include <algorithm>
#include <map>

namespace dbx4 {

struct Row {
    std::map<std::string, std::string> columns;
};

class SQLExecutor {
public:
    std::vector<Row> execute_select(
        const std::vector<Row>& rows,
        const std::vector<std::string>& projection,
        const std::string& order_by,
        int limit
    ) {
        std::vector<Row> result = rows;
        
        // Apply projection (select specific columns)
        if (!projection.empty()) {
            for (auto& row : result) {
                std::map<std::string, std::string> projected;
                for (const auto& col : projection) {
                    if (row.columns.count(col)) {
                        projected[col] = row.columns[col];
                    }
                }
                row.columns = projected;
            }
        }
        
        // Apply ORDER BY
        if (!order_by.empty()) {
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
        if (limit > 0 && result.size() > limit) {
            result.resize(limit);
        }
        
        return result;
    }
};

}
