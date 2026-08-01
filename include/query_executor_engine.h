#pragma once
#include "sql_parser.h"
#include "../include/dbx4_exceptions.h"
#include "../include/dbx4_logger.h"
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <any>

namespace dbx4 {

// Result row - map of column name to value
using Row = std::map<std::string, std::any>;

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
    std::map<std::string, size_t> column_index;  // column name -> index
};

// In-memory table storage
struct Table {
    TableSchema schema;
    std::vector<Row> rows;
    std::map<std::string, size_t> row_ids;  // Simple ID mapping
    uint64_t next_row_id = 0;
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
    
    // Execute methods for each statement type
    QueryResult execute_create_table(const std::shared_ptr<CreateTableStmt>& stmt);
    QueryResult execute_insert(const std::shared_ptr<InsertStmt>& stmt);
    QueryResult execute_select(const std::shared_ptr<SelectStmt>& stmt);
    QueryResult execute_update(const std::shared_ptr<UpdateStmt>& stmt);
    QueryResult execute_delete(const std::shared_ptr<DeleteStmt>& stmt);
    
    // Helper methods
    std::any evaluate_expression(const std::shared_ptr<Expression>& expr, const Row& row);
    bool evaluate_where_clause(const std::shared_ptr<Expression>& where, const Row& row);
    std::any convert_value(const std::string& value, TokenType type);
    std::string type_to_string(TokenType type);
    
public:
    explicit QueryExecutor(const std::string& db_name)
        : db_(std::make_shared<Database>(db_name)) {}

    // Main execute method
    QueryResult execute(const std::shared_ptr<ASTNode>& ast);
    
    // Utility methods
    std::shared_ptr<Database> get_database() const { return db_; }
};

} // namespace dbx4

