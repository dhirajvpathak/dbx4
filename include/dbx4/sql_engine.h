#ifndef DBX4_SQL_ENGINE_H
#define DBX4_SQL_ENGINE_H

#include <string>
#include <vector>
#include <map>
#include <set>
#include <memory>
#include <algorithm>
#include <cctype>
#include <sstream>
#include <cmath>

namespace dbx4 {

// Column definition
struct Column {
    std::string name;
    std::string type;  // INT, VARCHAR, DECIMAL, TIMESTAMP, BOOLEAN
    bool nullable = true;
    std::string default_value;
};

// Row data
using Row = std::map<std::string, std::string>;

// SQL comparison operators
enum class ComparisonOp {
    EQUAL,
    NOT_EQUAL,
    LESS_THAN,
    LESS_EQUAL,
    GREATER_THAN,
    GREATER_EQUAL,
    IN,
    NOT_IN,
    LIKE,
    NOT_LIKE,
    IS_NULL,
    IS_NOT_NULL,
    BETWEEN
};

// Logical operators
enum class LogicalOp {
    AND,
    OR,
    NOT
};

// ORDER BY direction
enum class OrderDirection {
    ASC,
    DESC
};

// JOIN type
enum class JoinType {
    INNER,
    LEFT,
    RIGHT
};

// WHERE clause condition
struct Condition {
    std::string column;
    ComparisonOp op;
    std::string value;
    std::vector<std::string> values;  // For IN operator
    LogicalOp logical_op = LogicalOp::AND;
    std::shared_ptr<Condition> left;
    std::shared_ptr<Condition> right;
    
    bool evaluate(const Row& row) const;
};

// ORDER BY clause
struct OrderByClause {
    std::string column;
    OrderDirection direction = OrderDirection::ASC;
};

// GROUP BY aggregation
struct AggregationFunc {
    std::string type;  // COUNT, SUM, AVG, MIN, MAX
    std::string column;
};

// SELECT statement structure
struct SelectStatement {
    std::vector<std::string> select_columns;  // Empty = SELECT *
    std::string from_table;
    std::shared_ptr<Condition> where_clause;
    std::vector<std::string> group_by_columns;
    std::shared_ptr<Condition> having_clause;
    std::vector<OrderByClause> order_by;
    int limit = -1;  // -1 = no limit
    int offset = 0;
    bool distinct = false;
};

// SQL parser
class SQLParser {
public:
    static SelectStatement parse_select(const std::string& sql);
    static std::shared_ptr<Condition> parse_where(const std::string& where_clause);
    
private:
    static std::vector<std::string> tokenize(const std::string& sql);
    static std::string to_upper(const std::string& s);
    static std::string trim(const std::string& s);
};

// SQL executor
class SQLExecutor {
public:
    SQLExecutor();
    
    // Execute SELECT query
    std::vector<Row> execute_select(const SelectStatement& stmt, 
                                   const std::vector<Row>& table_data);
    
    // Filter rows with WHERE clause
    std::vector<Row> filter_where(const std::vector<Row>& rows,
                                 const std::shared_ptr<Condition>& condition);
    
    // Project specific columns
    std::vector<Row> project_columns(const std::vector<Row>& rows,
                                    const std::vector<std::string>& columns);
    
    // Sort rows with ORDER BY
    std::vector<Row> order_by(std::vector<Row> rows,
                             const std::vector<OrderByClause>& order_clauses);
    
    // Apply LIMIT and OFFSET
    std::vector<Row> apply_limit_offset(const std::vector<Row>& rows,
                                       int limit, int offset);
    
    // GROUP BY with aggregation
    std::vector<Row> group_by(const std::vector<Row>& rows,
                             const std::vector<std::string>& group_columns,
                             const std::vector<AggregationFunc>& aggregations);
    
    // DISTINCT
    std::vector<Row> distinct(const std::vector<Row>& rows);
    
    // JOIN operations
    std::vector<Row> inner_join(const std::vector<Row>& left_rows,
                               const std::vector<Row>& right_rows,
                               const std::string& join_condition);
    
    std::vector<Row> left_join(const std::vector<Row>& left_rows,
                              const std::vector<Row>& right_rows,
                              const std::string& join_condition);
    
private:
    // Helper functions
    bool matches_pattern(const std::string& value, const std::string& pattern);
    int compare_values(const std::string& a, const std::string& b);
    std::string get_aggregate_value(const std::vector<Row>& group_rows,
                                   const AggregationFunc& agg);
};

}  // namespace dbx4

#endif  // DBX4_SQL_ENGINE_H
