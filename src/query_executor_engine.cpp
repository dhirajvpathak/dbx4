#include "query_executor_engine_FIXED.h"
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
    
    if (columns.empty()) {
        throw SQLValidationException("Table must have at least one column");
    }
    
    auto table = std::make_shared<Table>();
    table->schema.name = name;
    table->schema.columns = columns;
    
    for (size_t i = 0; i < columns.size(); ++i) {
        table->schema.column_index[columns[i].name] = i;
        
        if (columns[i].primary_key) {
            table->schema.primary_keys.insert(columns[i].name);
        }
        if (columns[i].unique) {
            table->schema.unique_columns.insert(columns[i].name);
        }
        if (columns[i].not_null) {
            table->schema.not_null_columns.insert(columns[i].name);
        }
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
        // Validate column count
        if (!stmt->columns.empty()) {
            // Explicit column list provided
            if (value_row.size() != stmt->columns.size()) {
                throw SQLValidationException("Column count mismatch: expected " + 
                    std::to_string(stmt->columns.size()) + ", got " + std::to_string(value_row.size()));
            }
        } else {
            // No explicit columns - must match table structure
            if (value_row.size() != table->schema.columns.size()) {
                throw SQLValidationException("Column count mismatch: expected " + 
                    std::to_string(table->schema.columns.size()) + ", got " + std::to_string(value_row.size()));
            }
        }
        
        Row new_row;
        
        // Build row with proper column mapping
        if (!stmt->columns.empty()) {
            // Explicit columns
            for (size_t i = 0; i < stmt->columns.size(); ++i) {
                const std::string& col_name = stmt->columns[i];
                validate_column_exists(col_name, table);
                
                size_t col_idx = table->schema.column_index[col_name];
                TokenType col_type = table->schema.columns[col_idx].type;
                
                if (auto literal = std::dynamic_pointer_cast<Literal>(value_row[i])) {
                    Value val = convert_to_type(
                        Value(literal->value), 
                        literal->type == TokenType::STRING ? TokenType::VARCHAR : literal->type
                    );
                    new_row[col_name] = val;
                } else {
                    throw SQLValidationException("Only literal values supported in INSERT");
                }
            }
            
            // Add default values for missing columns
            for (size_t col_idx = 0; col_idx < table->schema.columns.size(); ++col_idx) {
                const auto& col = table->schema.columns[col_idx];
                if (new_row.find(col.name) == new_row.end()) {
                    if (!col.default_value.empty()) {
                        new_row[col.name] = convert_to_type(Value(col.default_value), col.type);
                    } else if (!col.not_null) {
                        new_row[col.name] = Value();  // NULL
                    } else {
                        throw NotNullException(col.name);
                    }
                }
            }
        } else {
            // No explicit columns - use table order
            for (size_t i = 0; i < value_row.size(); ++i) {
                const auto& col = table->schema.columns[i];
                TokenType col_type = col.type;
                
                if (auto literal = std::dynamic_pointer_cast<Literal>(value_row[i])) {
                    Value val = convert_to_type(
                        Value(literal->value),
                        literal->type == TokenType::STRING ? TokenType::VARCHAR : literal->type
                    );
                    new_row[col.name] = val;
                } else {
                    throw SQLValidationException("Only literal values supported in INSERT");
                }
            }
        }
        
        // Validate constraints
        validate_row(new_row, table);
        
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
                for (const auto& col : table->schema.columns) {
                    select_columns.push_back(col.name);
                }
            } else {
                validate_column_exists(id->name, table);
                select_columns.push_back(id->name);
            }
        }
    } else {
        for (const auto& col_expr : stmt->columns) {
            if (auto id = std::dynamic_pointer_cast<Identifier>(col_expr)) {
                validate_column_exists(id->name, table);
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
                    const std::string& col_name = order_col.first;
                    bool is_desc = order_col.second;
                    
                    validate_column_exists(col_name, table);
                    
                    const Value& a_val = a.at(col_name);
                    const Value& b_val = b.at(col_name);
                    
                    if (a_val.is_null() && b_val.is_null()) continue;
                    if (a_val.is_null()) return !is_desc;
                    if (b_val.is_null()) return is_desc;
                    
                    int cmp = 0;
                    if (a_val.type == Value::INT_VAL && b_val.type == Value::INT_VAL) {
                        auto a_i = std::any_cast<int64_t>(a_val.data);
                        auto b_i = std::any_cast<int64_t>(b_val.data);
                        cmp = (a_i < b_i) ? -1 : (a_i > b_i) ? 1 : 0;
                    } else if (a_val.type == Value::DOUBLE_VAL && b_val.type == Value::DOUBLE_VAL) {
                        auto a_d = std::any_cast<double>(a_val.data);
                        auto b_d = std::any_cast<double>(b_val.data);
                        cmp = (a_d < b_d) ? -1 : (a_d > b_d) ? 1 : 0;
                    } else if (a_val.type == Value::STRING_VAL && b_val.type == Value::STRING_VAL) {
                        auto a_s = std::any_cast<std::string>(a_val.data);
                        auto b_s = std::any_cast<std::string>(b_val.data);
                        cmp = a_s.compare(b_s);
                    }
                    
                    if (cmp != 0) {
                        return is_desc ? (cmp > 0) : (cmp < 0);
                    }
                }
                return false;
            });
    }
    
    // Apply LIMIT and OFFSET (FIX: LIMIT 0 returns empty, not all rows)
    int start_idx = stmt->offset;
    int end_idx = (stmt->limit > 0) ? (start_idx + stmt->limit) : filtered_rows.size();
    
    if (stmt->limit == 0) {
        end_idx = start_idx;  // LIMIT 0 returns no rows
    }
    
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
            
            validate_column_exists(col_name, table);
            
            Value new_val = evaluate_expression(value_expr, row);
            row[col_name] = new_val;
        }
        
        validate_row(row, table);
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

Value QueryExecutor::evaluate_expression(const std::shared_ptr<Expression>& expr, const Row& row) {
    if (auto literal = std::dynamic_pointer_cast<Literal>(expr)) {
        if (literal->type == TokenType::NULL_KEYWORD) {
            return Value();
        }
        return Value(literal->value);
    }
    
    if (auto id = std::dynamic_pointer_cast<Identifier>(expr)) {
        if (row.find(id->name) != row.end()) {
            return row.at(id->name);
        }
        throw SQLValidationException("Unknown column: '" + id->name + "'");
    }
    
    if (auto unary = std::dynamic_pointer_cast<UnaryOp>(expr)) {
        auto operand = evaluate_expression(unary->operand, row);
        
        if (unary->op == TokenType::MINUS) {
            if (operand.type == Value::INT_VAL) {
                return Value(-std::any_cast<int64_t>(operand.data));
            } else if (operand.type == Value::DOUBLE_VAL) {
                return Value(-std::any_cast<double>(operand.data));
            }
            throw TypeException("Cannot negate non-numeric value");
        }
        return operand;
    }
    
    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(expr)) {
        auto left = evaluate_expression(binop->left, row);
        auto right = evaluate_expression(binop->right, row);
        
        if (left.is_null() || right.is_null()) {
            return Value();  // NULL
        }
        
        switch (binop->op) {
            case TokenType::PLUS: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return Value(std::any_cast<int64_t>(left.data) + std::any_cast<int64_t>(right.data));
                }
                double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                return Value(l + r);
            }
            case TokenType::MINUS: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return Value(std::any_cast<int64_t>(left.data) - std::any_cast<int64_t>(right.data));
                }
                double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                return Value(l - r);
            }
            case TokenType::MULTIPLY: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return Value(std::any_cast<int64_t>(left.data) * std::any_cast<int64_t>(right.data));
                }
                double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                return Value(l * r);
            }
            case TokenType::DIVIDE: {
                double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                if (r == 0) throw OutOfRangeException("0", "divisor");
                return Value(l / r);
            }
            default:
                throw SQLValidationException("Unsupported operator in expression");
        }
    }
    
    throw SQLValidationException("Unable to evaluate expression");
}

bool QueryExecutor::evaluate_where_clause(const std::shared_ptr<Expression>& where, const Row& row) {
    if (auto binop = std::dynamic_pointer_cast<BinaryOp>(where)) {
        // Short-circuit AND/OR (FIX: now properly evaluates both sides)
        if (binop->op == TokenType::AND) {
            bool left_result = evaluate_where_clause(binop->left, row);
            if (!left_result) return false;  // Short-circuit
            return evaluate_where_clause(binop->right, row);
        }
        if (binop->op == TokenType::OR) {
            bool left_result = evaluate_where_clause(binop->left, row);
            if (left_result) return true;  // Short-circuit
            return evaluate_where_clause(binop->right, row);
        }
        
        auto left = evaluate_expression(binop->left, row);
        auto right = evaluate_expression(binop->right, row);
        
        if (left.is_null() || right.is_null()) {
            return false;  // NULL comparison is always false
        }
        
        switch (binop->op) {
            case TokenType::EQUALS: {
                if (left.type != right.type) {
                    // Type mismatch in comparison
                    return false;
                }
                if (left.type == Value::INT_VAL) {
                    return std::any_cast<int64_t>(left.data) == std::any_cast<int64_t>(right.data);
                } else if (left.type == Value::DOUBLE_VAL) {
                    return std::any_cast<double>(left.data) == std::any_cast<double>(right.data);
                } else if (left.type == Value::STRING_VAL) {
                    return std::any_cast<std::string>(left.data) == std::any_cast<std::string>(right.data);
                } else if (left.type == Value::BOOL_VAL) {
                    return std::any_cast<bool>(left.data) == std::any_cast<bool>(right.data);
                }
                return false;
            }
            case TokenType::NOT_EQUALS:
                return !evaluate_where_clause(
                    std::make_shared<BinaryOp>(binop->left, binop->right, TokenType::EQUALS), row);
            case TokenType::LESS: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return std::any_cast<int64_t>(left.data) < std::any_cast<int64_t>(right.data);
                } else if (left.type == Value::DOUBLE_VAL || right.type == Value::DOUBLE_VAL) {
                    double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                    double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                    return l < r;
                }
                return false;
            }
            case TokenType::LESS_EQUAL: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return std::any_cast<int64_t>(left.data) <= std::any_cast<int64_t>(right.data);
                } else if (left.type == Value::DOUBLE_VAL || right.type == Value::DOUBLE_VAL) {
                    double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                    double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                    return l <= r;
                }
                return false;
            }
            case TokenType::GREATER: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return std::any_cast<int64_t>(left.data) > std::any_cast<int64_t>(right.data);
                } else if (left.type == Value::DOUBLE_VAL || right.type == Value::DOUBLE_VAL) {
                    double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                    double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                    return l > r;
                }
                return false;
            }
            case TokenType::GREATER_EQUAL: {
                if (left.type == Value::INT_VAL && right.type == Value::INT_VAL) {
                    return std::any_cast<int64_t>(left.data) >= std::any_cast<int64_t>(right.data);
                } else if (left.type == Value::DOUBLE_VAL || right.type == Value::DOUBLE_VAL) {
                    double l = (left.type == Value::INT_VAL) ? std::any_cast<int64_t>(left.data) : std::any_cast<double>(left.data);
                    double r = (right.type == Value::INT_VAL) ? std::any_cast<int64_t>(right.data) : std::any_cast<double>(right.data);
                    return l >= r;
                }
                return false;
            }
            default:
                throw SQLValidationException("Unsupported operator in WHERE clause");
        }
    }
    
    throw SQLValidationException("Invalid WHERE clause");
}

Value QueryExecutor::convert_to_type(const Value& value, TokenType target_type) {
    if (value.is_null()) return Value();
    
    switch (target_type) {
        case TokenType::INT:
        case TokenType::BIGINT: {
            if (value.type == Value::INT_VAL) return value;
            if (value.type == Value::DOUBLE_VAL) {
                return Value(static_cast<int64_t>(std::any_cast<double>(value.data)));
            }
            if (value.type == Value::STRING_VAL) {
                try {
                    return Value(std::stoll(std::any_cast<std::string>(value.data)));
                } catch (...) {
                    throw TypeCastException(value.to_string(), token_type_to_string(target_type));
                }
            }
            break;
        }
        case TokenType::DOUBLE:
        case TokenType::FLOAT: {
            if (value.type == Value::DOUBLE_VAL) return value;
            if (value.type == Value::INT_VAL) {
                return Value(static_cast<double>(std::any_cast<int64_t>(value.data)));
            }
            if (value.type == Value::STRING_VAL) {
                try {
                    return Value(std::stod(std::any_cast<std::string>(value.data)));
                } catch (...) {
                    throw TypeCastException(value.to_string(), token_type_to_string(target_type));
                }
            }
            break;
        }
        case TokenType::VARCHAR:
        case TokenType::TEXT:
        case TokenType::CHAR:
            return Value(value.to_string());
        case TokenType::BOOLEAN: {
            if (value.type == Value::BOOL_VAL) return value;
            if (value.type == Value::INT_VAL) {
                return Value(std::any_cast<int64_t>(value.data) != 0);
            }
            if (value.type == Value::STRING_VAL) {
                std::string s = std::any_cast<std::string>(value.data);
                return Value(s == "true" || s == "1" || s == "TRUE" || s == "YES");
            }
            break;
        }
        default:
            break;
    }
    
    return value;
}

std::string QueryExecutor::token_type_to_string(TokenType type) {
    switch (type) {
        case TokenType::INT: return "INT";
        case TokenType::BIGINT: return "BIGINT";
        case TokenType::DOUBLE: return "DOUBLE";
        case TokenType::FLOAT: return "FLOAT";
        case TokenType::VARCHAR: return "VARCHAR";
        case TokenType::CHAR: return "CHAR";
        case TokenType::BOOLEAN: return "BOOLEAN";
        case TokenType::TIMESTAMP: return "TIMESTAMP";
        case TokenType::TEXT: return "TEXT";
        default: return "UNKNOWN";
    }
}

void QueryExecutor::validate_row(const Row& row, const std::shared_ptr<Table>& table) {
    // Check NOT NULL constraints
    for (const auto& col_name : table->schema.not_null_columns) {
        if (row.find(col_name) != row.end() && row.at(col_name).is_null()) {
            throw NotNullException(col_name);
        }
    }
    
    // Check UNIQUE constraints
    for (const auto& col_name : table->schema.unique_columns) {
        if (row.find(col_name) == row.end()) continue;
        
        const Value& val = row.at(col_name);
        if (val.is_null()) continue;  // NULL values don't violate UNIQUE
        
        for (const auto& existing_row : table->rows) {
            if (existing_row.find(col_name) != existing_row.end()) {
                const Value& existing_val = existing_row.at(col_name);
                if (!existing_val.is_null() && val.to_string() == existing_val.to_string()) {
                    throw UniqueConstraintException(col_name);
                }
            }
        }
    }
}

void QueryExecutor::validate_column_exists(const std::string& col_name, const std::shared_ptr<Table>& table) {
    if (table->schema.column_index.find(col_name) == table->schema.column_index.end()) {
        throw SQLValidationException("Column '" + col_name + "' does not exist in table '" + table->schema.name + "'");
    }
}

} // namespace dbx4

