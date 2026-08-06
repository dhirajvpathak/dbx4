#pragma once
#include "sql_parser.h"
#include "../include/dbx4_exceptions.h"
#include "../include/dbx4_logger.h"
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <any>
#include <optional>
#include <set>

namespace dbx4 {

// Value type - supports multiple types
struct Value {
    enum Type { NULL_VAL, INT_VAL, DOUBLE_VAL, STRING_VAL, BOOL_VAL };
    
    Type type;
    std::any data;
    
    Value() : type(NULL_VAL) {}
    Value(int64_t v) : type(INT_VAL), data(v) {}
    Value(double v) : type(DOUBLE_VAL), data(v) {}
    Value(const std::string& v) : type(STRING_VAL), data(v) {}
    Value(bool v) : type(BOOL_VAL), data(v) {}
    
    bool is_null() const { return type == NULL_VAL; }
    
    std::string to_string() const {
        if (is_null()) return "NULL";
        try {
            switch (type) {
                case INT_VAL: return std::to_string(std::any_cast<int64_t>(data));
                case DOUBLE_VAL: {
                    auto d = std::any_cast<double>(data);
                    if (d == static_cast<int64_t>(d)) {
                        return std::to_string(static_cast<int64_t>(d));
                    }
                    return std::to_string(d);
                }
                case STRING_VAL: return std::any_cast<std::string>(data);
                case BOOL_VAL: return std::any_cast<bool>(data) ? "true" : "false";
                default: return "";
            }
        } catch (...) {
            return "";
        }
    }
};

// Result row - map of column name to value
using Row = std::map<std::string, Value>;

// Query result set
struct QueryResult {
    std::vector<std::string> column_names;
    std::vector<Row> rows;
    int rows_affected = 0;
    bool success = false;
    std::string error_message;
};

// Table schema definition
struct TableSchema {
    std::string name;
    std::vector<ColumnDef> columns;
    std::map<std::string, size_t> column_index;
    std::set<std::string> primary_keys;
    std::set<std::string> unique_columns;
    std::set<std::string> not_null_columns;
};

// In-memory table storage
struct Table {
    TableSchema schema;
    std::vector<Row> rows;
    std::map<std::string, std::set<Value>> unique_indexes;  // Track unique constraint values
};

// Database - collection of tables
class Database {
private:
    std::map<std::string, std::shared_ptr<Table>> tables_;
    std::string name_;

public:
    explicit Database(const std::string& name) : name_(name) {}

    std::shared_ptr<Table> get_table(const std::string& name);
    std::shared_ptr<Table> create_table(const std::string& name, const std::vector<ColumnDef>& columns);
    bool table_exists(const std::string& name);
    void drop_table(const std::string& name);
    const std::string& get_name() const { return name_; }
};

// Query Executor - executes parsed SQL statements
class QueryExecutor {
private:
    std::shared_ptr<Database> db_;
    
    QueryResult execute_create_table(const std::shared_ptr<CreateTableStmt>& stmt);
    QueryResult execute_insert(const std::shared_ptr<InsertStmt>& stmt);
    QueryResult execute_select(const std::shared_ptr<SelectStatement>& stmt);
    QueryResult execute_update(const std::shared_ptr<UpdateStmt>& stmt);
    QueryResult execute_delete(const std::shared_ptr<DeleteStmt>& stmt);
    
    // Helper methods
    Value evaluate_expression(const std::shared_ptr<Expression>& expr, const Row& row);
    bool evaluate_where_clause(const std::shared_ptr<Expression>& where, const Row& row);
    Value convert_to_type(const Value& value, TokenType target_type);
    std::string token_type_to_string(TokenType type);
    
    // Validation
    void validate_row(const Row& row, const std::shared_ptr<Table>& table);
    void validate_column_exists(const std::string& col_name, const std::shared_ptr<Table>& table);
    
public:
    explicit QueryExecutor(const std::string& db_name)
        : db_(std::make_shared<Database>(db_name)) {}

    QueryResult execute(const std::shared_ptr<ASTNode>& ast);
    std::shared_ptr<Database> get_database() const { return db_; }
};

} // namespace dbx4

