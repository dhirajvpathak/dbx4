#include "../include/sql_parser.h"
#include <algorithm>

namespace dbx4 {

Token SQLParser::current() {
    if (pos_ >= tokens_.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens_[pos_];
}

Token SQLParser::peek() {
    if (pos_ + 1 >= tokens_.size()) {
        return Token(TokenType::EOF_TOKEN, "", 0, 0);
    }
    return tokens_[pos_ + 1];
}

void SQLParser::advance() {
    if (pos_ < tokens_.size()) {
        pos_++;
    }
}

bool SQLParser::match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

bool SQLParser::check(TokenType type) {
    return current().type == type;
}

void SQLParser::consume(TokenType type, const std::string& message) {
    if (!check(type)) {
        throw SQLParseException(message + " at line " + std::to_string(current().line));
    }
    advance();
}

std::shared_ptr<ASTNode> SQLParser::parse() {
    auto stmt = parse_statement();
    
    // Require EOF or semicolon at end
    if (!check(TokenType::EOF_TOKEN) && !check(TokenType::SEMICOLON)) {
        throw SQLParseException("Expected end of statement, got unexpected token '" + 
                               current().value + "' at line " + std::to_string(current().line));
    }
    
    return stmt;
}

std::shared_ptr<ASTNode> SQLParser::parse_statement() {
    if (check(TokenType::CREATE)) {
        return parse_create_table();
    } else if (check(TokenType::SELECT)) {
        return parse_select();
    } else if (check(TokenType::INSERT)) {
        return parse_insert();
    } else if (check(TokenType::UPDATE)) {
        return parse_update();
    } else if (check(TokenType::DELETE)) {
        return parse_delete();
    }
    
    throw SQLParseException("Unknown statement type at line " + std::to_string(current().line));
}

std::shared_ptr<CreateTableStmt> SQLParser::parse_create_table() {
    auto stmt = std::make_shared<CreateTableStmt>();
    
    consume(TokenType::CREATE, "Expected CREATE");
    consume(TokenType::TABLE, "Expected TABLE");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw SQLParseException("Expected table name at line " + std::to_string(current().line));
    }
    stmt->table_name = current().value;
    advance();
    
    consume(TokenType::LPAREN, "Expected ( after table name");
    
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        if (!check(TokenType::IDENTIFIER)) {
            throw SQLParseException("Expected column name at line " + std::to_string(current().line));
        }
        
        ColumnDef col;
        col.name = current().value;
        advance();
        
        // Parse type (REQUIRED)
        if (check(TokenType::INT)) {
            col.type = TokenType::INT;
            advance();
        } else if (check(TokenType::BIGINT)) {
            col.type = TokenType::BIGINT;
            advance();
        } else if (check(TokenType::DOUBLE)) {
            col.type = TokenType::DOUBLE;
            advance();
        } else if (check(TokenType::VARCHAR)) {
            col.type = TokenType::VARCHAR;
            advance();
            if (match(TokenType::LPAREN)) {
                if (!check(TokenType::NUMBER)) {
                    throw SQLParseException("Expected number for VARCHAR length");
                }
                advance();
                consume(TokenType::RPAREN, "Expected ) after VARCHAR length");
            }
        } else if (check(TokenType::CHAR)) {
            col.type = TokenType::CHAR;
            advance();
        } else if (check(TokenType::BOOLEAN)) {
            col.type = TokenType::BOOLEAN;
            advance();
        } else if (check(TokenType::TIMESTAMP)) {
            col.type = TokenType::TIMESTAMP;
            advance();
        } else if (check(TokenType::TEXT)) {
            col.type = TokenType::TEXT;
            advance();
        } else {
            throw SQLParseException("Unknown data type '" + current().value + "' at line " + 
                                   std::to_string(current().line));
        }
        
        // Parse constraints
        while (check(TokenType::NOT_KEYWORD) || check(TokenType::PRIMARY) || 
               check(TokenType::UNIQUE) || check(TokenType::DEFAULT)) {
            if (match(TokenType::NOT_KEYWORD)) {
                consume(TokenType::NULL_KEYWORD, "Expected NULL after NOT");
                col.not_null = true;
            } else if (match(TokenType::PRIMARY)) {
                consume(TokenType::KEY, "Expected KEY after PRIMARY");
                col.primary_key = true;
            } else if (match(TokenType::UNIQUE)) {
                col.unique = true;
            } else if (match(TokenType::DEFAULT)) {
                if (!check(TokenType::STRING) && !check(TokenType::NUMBER) && 
                    !check(TokenType::NULL_KEYWORD)) {
                    throw SQLParseException("Expected default value at line " + std::to_string(current().line));
                }
                col.default_value = current().value;
                advance();
            }
        }
        
        stmt->columns.push_back(col);
        
        if (!check(TokenType::RPAREN)) {
            consume(TokenType::COMMA, "Expected , or )");
        }
    }
    
    if (stmt->columns.empty()) {
        throw SQLParseException("Table must have at least one column");
    }
    
    consume(TokenType::RPAREN, "Expected ) to close column list");
    match(TokenType::SEMICOLON);  // Optional
    
    return stmt;
}

std::shared_ptr<SelectStatement> SQLParser::parse_select() {
    auto stmt = std::make_shared<SelectStatement>();
    
    consume(TokenType::SELECT, "Expected SELECT");
    
    if (match(TokenType::DISTINCT)) {
        stmt->distinct = true;
    }
    
    // Parse column list
    do {
        if (check(TokenType::STAR)) {
            stmt->columns.push_back(std::make_shared<Identifier>("*"));
            stmt->column_aliases.push_back("");
            advance();
        } else {
            stmt->columns.push_back(parse_expression());
            // Check for alias
            if (match(TokenType::AS)) {
                if (!check(TokenType::IDENTIFIER)) {
                    throw SQLParseException("Expected alias name at line " + std::to_string(current().line));
                }
                stmt->column_aliases.push_back(current().value);
                advance();
            } else {
                stmt->column_aliases.push_back("");
            }
        }
    } while (match(TokenType::COMMA));
    
    if (stmt->columns.empty()) {
        throw SQLParseException("SELECT requires at least one column");
    }
    
    if (match(TokenType::FROM)) {
        if (!check(TokenType::IDENTIFIER)) {
            throw SQLParseException("Expected table name after FROM");
        }
        stmt->table_name = current().value;
        advance();
    }
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
        if (!stmt->where_clause) {
            throw SQLParseException("Invalid WHERE clause");
        }
    }
    
    if (match(TokenType::ORDER)) {
        consume(TokenType::BY, "Expected BY after ORDER");
        
        do {
            if (!check(TokenType::IDENTIFIER)) {
                throw SQLParseException("Expected column name in ORDER BY");
            }
            std::string col = current().value;
            advance();
            
            bool is_desc = false;
            if (match(TokenType::DESC)) {
                is_desc = true;
            } else {
                match(TokenType::ASC);
            }
            
            stmt->order_by.push_back({col, is_desc});
        } while (match(TokenType::COMMA));
    }
    
    if (match(TokenType::LIMIT)) {
        if (!check(TokenType::NUMBER)) {
            throw SQLParseException("Expected number after LIMIT");
        }
        try {
            stmt->limit = std::stoi(current().value);
            if (stmt->limit < 0) {
                throw SQLParseException("LIMIT must be non-negative");
            }
        } catch (const std::exception&) {
            throw SQLParseException("Invalid LIMIT value: " + current().value);
        }
        advance();
    }
    
    if (match(TokenType::OFFSET)) {
        if (!check(TokenType::NUMBER)) {
            throw SQLParseException("Expected number after OFFSET");
        }
        try {
            stmt->offset = std::stoi(current().value);
            if (stmt->offset < 0) {
                throw SQLParseException("OFFSET must be non-negative");
            }
        } catch (const std::exception&) {
            throw SQLParseException("Invalid OFFSET value: " + current().value);
        }
        advance();
    }
    
    return stmt;
}

std::shared_ptr<InsertStmt> SQLParser::parse_insert() {
    auto stmt = std::make_shared<InsertStmt>();
    
    consume(TokenType::INSERT, "Expected INSERT");
    consume(TokenType::INTO, "Expected INTO");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw SQLParseException("Expected table name");
    }
    stmt->table_name = current().value;
    advance();
    
    // Optional explicit column list
    if (match(TokenType::LPAREN)) {
        do {
            if (!check(TokenType::IDENTIFIER)) {
                throw SQLParseException("Expected column name in INSERT");
            }
            stmt->columns.push_back(current().value);
            advance();
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RPAREN, "Expected ) after column list");
    }
    
    consume(TokenType::VALUES, "Expected VALUES");
    
    do {
        consume(TokenType::LPAREN, "Expected ( for value row");
        
        std::vector<std::shared_ptr<Expression>> values;
        do {
            values.push_back(parse_expression());
        } while (match(TokenType::COMMA));
        
        consume(TokenType::RPAREN, "Expected ) to close value row");
        
        stmt->values.push_back(values);
    } while (match(TokenType::COMMA));
    
    if (stmt->values.empty()) {
        throw SQLParseException("INSERT requires at least one value row");
    }
    
    match(TokenType::SEMICOLON);  // Optional
    return stmt;
}

std::shared_ptr<UpdateStmt> SQLParser::parse_update() {
    auto stmt = std::make_shared<UpdateStmt>();
    
    consume(TokenType::UPDATE, "Expected UPDATE");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw SQLParseException("Expected table name");
    }
    stmt->table_name = current().value;
    advance();
    
    consume(TokenType::SET, "Expected SET");
    
    do {
        if (!check(TokenType::IDENTIFIER)) {
            throw SQLParseException("Expected column name in UPDATE");
        }
        std::string col = current().value;
        advance();
        
        consume(TokenType::EQUALS, "Expected = in assignment");
        
        auto expr = parse_expression();
        stmt->assignments.push_back({col, expr});
    } while (match(TokenType::COMMA));
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
    }
    
    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<DeleteStmt> SQLParser::parse_delete() {
    auto stmt = std::make_shared<DeleteStmt>();
    
    consume(TokenType::DELETE, "Expected DELETE");
    consume(TokenType::FROM, "Expected FROM");
    
    if (!check(TokenType::IDENTIFIER)) {
        throw SQLParseException("Expected table name");
    }
    stmt->table_name = current().value;
    advance();
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
    }
    
    match(TokenType::SEMICOLON);
    return stmt;
}

std::shared_ptr<Expression> SQLParser::parse_expression() {
    return parse_or_expression();
}

std::shared_ptr<Expression> SQLParser::parse_or_expression() {
    auto expr = parse_and_expression();
    
    while (match(TokenType::OR)) {
        auto right = parse_and_expression();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::OR);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_and_expression() {
    auto expr = parse_comparison();
    
    while (match(TokenType::AND)) {
        auto right = parse_comparison();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::AND);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_comparison() {
    auto expr = parse_additive();
    
    if (match(TokenType::EQUALS)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::EQUALS);
    } else if (match(TokenType::NOT_EQUALS)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::NOT_EQUALS);
    } else if (match(TokenType::LESS)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::LESS);
    } else if (match(TokenType::LESS_EQUAL)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::LESS_EQUAL);
    } else if (match(TokenType::GREATER)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::GREATER);
    } else if (match(TokenType::GREATER_EQUAL)) {
        auto right = parse_additive();
        expr = std::make_shared<BinaryOp>(expr, right, TokenType::GREATER_EQUAL);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_additive() {
    auto expr = parse_multiplicative();
    
    while (check(TokenType::PLUS) || check(TokenType::MINUS)) {
        TokenType op = current().type;
        advance();
        auto right = parse_multiplicative();
        expr = std::make_shared<BinaryOp>(expr, right, op);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_multiplicative() {
    auto expr = parse_unary();
    
    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE) || check(TokenType::MODULO)) {
        TokenType op = current().type;
        advance();
        auto right = parse_unary();
        expr = std::make_shared<BinaryOp>(expr, right, op);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_unary() {
    if (match(TokenType::NOT_OP)) {
        auto expr = parse_unary();
        return std::make_shared<UnaryOp>(expr, TokenType::NOT_OP);
    }
    
    if (match(TokenType::MINUS)) {
        auto expr = parse_unary();
        return std::make_shared<UnaryOp>(expr, TokenType::MINUS);
    }
    
    return parse_primary();
}

std::shared_ptr<Expression> SQLParser::parse_primary() {
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        consume(TokenType::RPAREN, "Expected ) after expression");
        return expr;
    }
    
    if (check(TokenType::STRING)) {
        auto lit = std::make_shared<Literal>(current().value, TokenType::STRING);
        advance();
        return lit;
    }
    
    if (check(TokenType::NUMBER)) {
        auto lit = std::make_shared<Literal>(current().value, TokenType::NUMBER);
        advance();
        return lit;
    }
    
    if (check(TokenType::NULL_KEYWORD)) {
        auto lit = std::make_shared<Literal>("NULL", TokenType::NULL_KEYWORD);
        advance();
        return lit;
    }
    
    if (check(TokenType::IDENTIFIER)) {
        auto id = std::make_shared<Identifier>(current().value);
        advance();
        return id;
    }
    
    throw SQLParseException("Unexpected token '" + current().value + 
                           "' in expression at line " + std::to_string(current().line));
}

} // namespace dbx4

