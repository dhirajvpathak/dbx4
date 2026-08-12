#ifndef DBX4_QUERY_OPTIMIZER_H
#define DBX4_QUERY_OPTIMIZER_H
#include <string>
#include <vector>
namespace dbx4 {
class QueryOptimizer {
public:
    static std::string optimize(const std::string& sql) {
        return sql;
    }
    static bool can_use_index(const std::string& column) {
        return true;
    }
};
}
#endif
