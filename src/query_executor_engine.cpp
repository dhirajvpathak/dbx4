#include "query_executor_engine.h"
#include <algorithm>
#include <cmath>
#include <sstream>

namespace dbx4 {

// Database implementation
std::shared_ptr<Table> Database::get_table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw StorageException("Table '" + name + "' does not exist");
    }
    return it->second;
}

std::shared_ptr<Table> Database::create_table(const std::string& name, const std::vector<ColumnDef>& columns) {
    if (table_exists(name)) {
        throw StorageException("Table '" + name + "' already exists");
    }
    
    auto table = std::make_shared<Table>();
    table->schema.name = name;
    table->schema.columns = columns;
    
    for (size_t i = 0; i < columns.size(); ++i) {
        table->schema.column_index[columns[i].name] = i;
    }
    
    tables_[name] = table;
    LOG_INFO("Database", "Table '" + name + "' created with " + std::to_string(columns.size()) + " columns");
    
    return table;
}

bool Database::table_exists(const std::string& name) {
    return tables_.find(name) != tables_.end();
}

void Database::drop_table(const std::string& name) {
    auto it = tables_.find(name);
    if (it == tables_.end()) {
        throw StorageException("Table '" + name + "' does not exist");
    }
    tables_.erase(it);
    LOG_INFO("Database", "Table '" + name + "' dropped");
}

// QueryExecutor implementation
QueryResult QueryExecutor::execute(const std::shared_ptr<ASTNode>& ast) {
    QueryResult result;
    result.success = false;
    
    try {
        if (auto create_stmt = std::dynamic_pointer_cast<CreateTableStmt>(ast)) {
            result = execute_create_table(create_stmt);
        } else if (auto insert_stmt = std::dynamic_pointer_cast<InsertStmt>(ast)) {
            result = execute_insert(insert_stmt);
        } else if (auto select_stmt = std::dynamic_pointer_cast<SelectStmt>(ast)) {
            result = execute_select(select_stmt);
        } else if (auto update_stmt = std::dynamic_pointer_cast<UpdateStmt>(ast)) {
            result = execute_update(update_stmt);
        } else if (auto delete_stmt = std::dynamic_pointer_cast<DeleteStmt>(ast)) {
            result = execute_delete(delete_stmt);
        } else {
            throw SQLValidationException("Unknown statement type");
        }
    } catch (const DBX4Exception& e) {
        result.success = false;
        result.error_message = e.what();
        LOG_ERROR("QueryExecutor", result.error_message);
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = std::string("Execution error: ") + e.what();
        LOG_ERROR("QueryExecutor", result.error_message);
    }
    
    return result;
}

QueryResult QueryExecutor::execute_create_table(const std::shared_ptr<CreateTableStmt>& stmt) {
    QueryResult result;
    
    if (stmt->table_name.empty()) {
        throw SQLValidationException("Table name cannot be empty");
    }
    
    if (stmt->columns.empty()) {
        throw SQLValidationException("Table must have at least one column");
    }
    
    db_->create_table(stmt->table_name, stmt->columns);
    
    result.success = true;
    result.rows_affected = 0;
    return result;
}

QueryResult QueryExecutor::execute_insert(const std::shared_ptr<InsertStmt>& stmt) {
    QueryResult result;
    
    if (stmt->table_name.empty()) {
        throw SQLValidationException("Table name cannot be empty");
    }
    
    auto table = db_->get_table(stmt->table_name);
    int rows_inserted = 0;
    
    for (const auto& value_row : stmt->values) {
        if (value_row.size() != table->schema.columns.size()) {
            throw SQLValidationException("Column count mismatch: expected " + 
                std::to_string(table->schema.columns.size()) + 
                ", got " + std::to_string(value_row.size()));
        }
        
        Row new_row;
        for (size_t i = 0; i < value_row.size(); ++i) {
            std::string col_name = table->schema.columns[i].name;
            TokenType col_type = table->schema.columns[i].type;
            
            if (auto literal = std::dynamic_pointer_cast<Literal>(value_row[i])) {
                new_row[col_name] = convert_value(literal->value, literal->type);
            } else {
                throw SQLValidationException("Only literal values supported in INSERT");
            }
        }
        
        table->rows.push_back(new_row);
        rows_inserted++;
    }
    
    result.success = true;
    result.rows_affected = rows_inserted;
    LOG_INFO("QueryExecutor", "Inserted " + std::to_string(rows_inserted) + " rows into '" + stmt->table_name + "'");
    
    return result;
}

QueryResult QueryExecutor::execute_select(const std::shared_ptr<SelectStmt>& stmt) {
    QueryResult result;
    
    if (stmt->table_name.empty()) {
        throw SQLValidationException("SELECT requires FROM clause");
    }
    
    auto table = db_->get_table(stmt->table_name);
    
    // Determine which columns to select
    std::vector<std::string> select_columns;
    if (stmt->columns.size() == 1) {
        if (auto id = std::dynamic_pointer_cast<Identifier>(stmt->columns[0])) {
            if (id->name == "*") {
                // Select all columns
                for (const auto& col : table->schema.columns) {
                    select_columns.push_back(col.name);
                }
            } else {
                select_columns.push_back(id->name);
            }
        }
    } else {
        for (const auto& col_expr : stmt->columns) {
            if (auto id = std::dynamic_pointer_cast<Identifier>(col_expr)) {
                select_columns.push_back(id->name);
            }
        }
    }
    
    result.column_names = select_columns;
    
    // Filter rows based on WHERE clause
    std::vector<Row> filtered_rows;
    for (const auto& row : table->rows) {
        if (stmt->where_clause) {
            if (evaluate_where_clause(stmt->where_clause, row)) {
                filtered_rows.push_back(row);
            }
        } else {
            filtered_rows.push_back(row);
        }
    }
    
    // Apply ORDER BY
    if (!stmt->order_by.empty()) {
        std::sort(filtered_rows.begin(), filtered_rows.end(),
            [&](const Row& a, const Row& b) {
                for (const auto& order_col : stmt->order_by) {
                    std::string col_name = order_col.first;
                    bool is_desc = order_col.second;
                    
                    if (a.find(col_name) == a.end() || b.find(col_name) == b.end()) {
                        continue;
                    }
                    
                    // Simple comparison for numbers
                    try {
                        double a_val = std::any_cast<double>(a.at(col_name));
                        double b_val = std::any_cast<double>(b.at(col_name));
                        
                        if (a_val != b_val) {
                            return is_desc ? (a_val > b_val) : (a_val < b_val);
                        }
                    } catch (...) {
                        // Try string comparison
                        try {
                            std::string a_val = std::any_cast<std::string>(a.at(col_name));
                            std::string b_val = std::any_cast<std::string>(b.at(col_name));
                            
                            if (a_val != b_val) {
                                return is_desc ? (a_val > b_val) : (a_val < b_val);
                            }
                        } catch (...) {}
                    }
                }
                return false;
            });
    }
    
    // Apply LIMIT and OFFSET
    int start_idx = stmt->offset;
    int end_idx = (stmt->limit > 0) ? (start_idx + stmt->limit) : filtered_rows.size();
    
    for (int i = start_idx; i < end_idx && i < static_cast<int>(filtered_rows.size()); ++i) {
        Row result_row;
        for (const auto& col_name : select_columns) {
            if (filtered_rows[i].find(col_name) != filtered_rows[i].end()) {
                result_row[col_name] = filtered_rows[i][col_name];
            }
        }
        result.rows.push_back(result_row);
    }
    
    result.success = true;
    result.rows_affected = result.rows.size();
    LOG_INFO("QueryExecutor", "SELECT returned " + std::to_string(result.rows.size()) + " rows");
    
    return result;
}

QueryResult QueryExecutor::execute_update(const std::shared_ptr<UpdateStmt>& stmt) {
    QueryResult result;
    
    if (stmt->table_name.empty()) {
        throw SQLValidationException("Table name cannot be empty");
    }
    
    auto table = db_->get_table(stmt->table_name);
    int rows_updated = 0;
    
    for (auto& row : table->rows) {
        if (stmt->where_clause) {
            if (!evaluate_where_clause(stmt->where_clause, row)) {
                continue;
            }
        }
        
        for (const auto& assignment : stmt->assignments) {
            const std::string& col_name = assignment.first;
            const auto& value_expr = assignment.second;
            
            if (row.find(col_name) == row.end()) {
                throw SQLValidationException("Column '" + col_name + "' does not exist");
            }
            
            row[col_name] = evaluate_expression(value_expr, row);
        }
        
        rows_updated++;
    }
    
    result.success = true;
    result.rows_affected = rows_updated;
    LOG_INFO("QueryExecutor", "Updated " + std::to_string(rows_updated) + " rows in '" + stmt->table_name + "'");
    
    return result;
}

QueryResult QueryExecutor::execute_delete(const std::shared_ptr<DeleteStmt>& stmt) {
    QueryResult result;
    
    if (stmt->table_name.empty()) {
        throw SQLValidationException("Table name cannot be empty");
    }
    
    auto table = db_->get_table(stmt->table_name);
    
    auto new_end = std::remove_if(table->rows.begin(), table->rows.end(),
        [this, &stmt](const Row& row) {
            if (stmt->where_clause) {
                return evaluate_where_clause(stmt->where_clause, row);
            }
            return true;  // Delete all if no WHERE clause
        });
    
    int rows_deleted = std::distance(new_end, table->rows.end());
    table->rows.erase(new_end, table->rows.end());
    
    result.success = true;
    result.rows_affected = rows_deleted;
    LOG_INFO("QueryExecutor", "Deleted " + std::to_string(rows_deleted) + " rows from '" + stmt->table_name + "'");
    
    return result;
}

std::any QueryExecutor::evaluate_expression(const std::shared_ptr<Expression>& expr, const Row& row) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expr)) {
        return convert_value(literal->value, literal->type);
    }
    
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        if (row.find(id->name) != row.end()) {
            return row.at(id->name);
        }
        throw SQLValidationException("Unknown column: " + id->name);
    }
    
    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        auto left = evaluate_expression(binop->left, row);
        auto right = evaluate_expression(binop->right, row);
        
        switch (binop->op) {
            case TokenType::PLUS: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l + r;
            }
            case TokenType::MINUS: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l - r;
            }
            case TokenType::MULTIPLY: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l * r;
            }
            case TokenType::DIVIDE: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                if (r == 0) throw OutOfRangeException("0", "divisor");
                return l / r;
            }
            default:
                throw SQLValidationException("Unsupported operator in expression");
        }
    }
    
    throw SQLValidationException("Unable to evaluate expression");
}

bool QueryExecutor::evaluate_where_clause(const std::shared_ptr<Expression>& where, const Row& row) {
    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(where)) {
        auto left = evaluate_expression(binop->left, row);
        auto right = evaluate_expression(binop->right, row);
        
        switch (binop->op) {
            case TokenType::EQUALS: {
                try {
                    double l = std::any_cast<double>(left);
                    double r = std::any_cast<double>(right);
                    return l == r;
                } catch (...) {
                    std::string l = std::any_cast<std::string>(left);
                    std::string r = std::any_cast<std::string>(right);
                    return l == r;
                }
            }
            case TokenType::NOT_EQUALS: {
                try {
                    double l = std::any_cast<double>(left);
                    double r = std::any_cast<double>(right);
                    return l != r;
                } catch (...) {
                    std::string l = std::any_cast<std::string>(left);
                    std::string r = std::any_cast<std::string>(right);
                    return l != r;
                }
            }
            case TokenType::LESS: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l < r;
            }
            case TokenType::LESS_EQUAL: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l <= r;
            }
            case TokenType::GREATER: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l > r;
            }
            case TokenType::GREATER_EQUAL: {
                double l = std::any_cast<double>(left);
                double r = std::any_cast<double>(right);
                return l >= r;
            }
            case TokenType::AND: {
                return evaluate_where_clause(binop->left, row) && 
                       evaluate_where_clause(binop->right, row);
            }
            case TokenType::OR: {
                return evaluate_where_clause(binop->left, row) || 
                       evaluate_where_clause(binop->right, row);
            }
            default:
                throw SQLValidationException("Unsupported operator in WHERE clause");
        }
    }
    
    throw SQLValidationException("Invalid WHERE clause");
}

std::any QueryExecutor::convert_value(const std::string& value, TokenType type) {
    try {
        switch (type) {
            case TokenType::NUMBER:
                return std::stod(value);
            case TokenType::STRING:
                return value;
            default:
                return value;
        }
    } catch (const std::exception& e) {
        throw TypeCastException(value, type_to_string(type));
    }
}

std::string QueryExecutor::type_to_string(TokenType type) {
    switch (type) {
        case TokenType::INT: return "INT";
        case TokenType::BIGINT: return "BIGINT";
        case TokenType::DOUBLE: return "DOUBLE";
        case TokenType::VARCHAR: return "VARCHAR";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::TIMESTAMP: return "TIMESTAMP";
        case TokenType::NUMBER: return "NUMBER";
        case TokenType::STRING: return "STRING";
        default: return "UNKNOWN";
    }
}

} // namespace dbx4

