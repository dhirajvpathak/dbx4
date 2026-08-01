#include "sql_parser.h"
#include "../include/dbx4_exceptions.h"

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

std::shared_ptr<ASTNode> SQLParser::parse() {
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
    
    throw SQLParseException("Unknown statement type");
}

std::shared_ptr<CreateTableStmt> SQLParser::parse_create_table() {
    auto stmt = std::make_shared<CreateTableStmt>();
    
    if (!match(TokenType::CREATE)) throw SQLParseException("Expected CREATE");
    if (!match(TokenType::TABLE)) throw SQLParseException("Expected TABLE");
    
    if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected table name");
    stmt->table_name = current().value;
    advance();
    
    if (!match(TokenType::LPAREN)) throw SQLParseException("Expected (");
    
    while (!check(TokenType::RPAREN) && !check(TokenType::EOF_TOKEN)) {
        if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected column name");
        
        ColumnDef col;
        col.name = current().value;
        advance();
        
        // Parse type
        if (check(TokenType::INT)) {
            col.type = TokenType::INT;
            advance();
        } else if (check(TokenType::VARCHAR)) {
            col.type = TokenType::VARCHAR;
            advance();
            if (match(TokenType::LPAREN)) {
                // Parse length
                if (!check(TokenType::NUMBER)) throw SQLParseException("Expected number for VARCHAR length");
                advance();
                if (!match(TokenType::RPAREN)) throw SQLParseException("Expected )");
            }
        } else if (check(TokenType::BIGINT)) {
            col.type = TokenType::BIGINT;
            advance();
        } else if (check(TokenType::DOUBLE)) {
            col.type = TokenType::DOUBLE;
            advance();
        } else if (check(TokenType::BOOLEAN)) {
            col.type = TokenType::BOOLEAN;
            advance();
        } else if (check(TokenType::TIMESTAMP)) {
            col.type = TokenType::TIMESTAMP;
            advance();
        } else {
            throw SQLParseException("Unknown data type");
        }
        
        // Parse constraints
        while (check(TokenType::NOT) || check(TokenType::PRIMARY) || check(TokenType::UNIQUE) || check(TokenType::DEFAULT)) {
            if (match(TokenType::NOT)) {
                if (!match(TokenType::NULL)) throw SQLParseException("Expected NULL after NOT");
                col.not_null = true;
            } else if (match(TokenType::PRIMARY)) {
                if (!match(TokenType::KEY)) throw SQLParseException("Expected KEY after PRIMARY");
                col.primary_key = true;
            } else if (match(TokenType::UNIQUE)) {
                col.unique = true;
            } else if (match(TokenType::DEFAULT)) {
                if (!check(TokenType::STRING) && !check(TokenType::NUMBER)) {
                    throw SQLParseException("Expected default value");
                }
                col.default_value = current().value;
                advance();
            }
        }
        
        stmt->columns.push_back(col);
        
        if (!check(TokenType::RPAREN)) {
            if (!match(TokenType::COMMA)) throw SQLParseException("Expected , or )");
        }
    }
    
    if (!match(TokenType::RPAREN)) throw SQLParseException("Expected )");
    if (match(TokenType::SEMICOLON)) {}  // Optional semicolon
    
    return stmt;
}

std::shared_ptr<SelectStmt> SQLParser::parse_select() {
    auto stmt = std::make_shared<SelectStmt>();
    
    if (!match(TokenType::SELECT)) throw SQLParseException("Expected SELECT");
    
    if (match(TokenType::DISTINCT)) {
        stmt->distinct = true;
    }
    
    // Parse column list
    do {
        if (check(TokenType::STAR)) {
            stmt->columns.push_back(std::make_shared<Identifier>("*"));
            advance();
        } else {
            stmt->columns.push_back(parse_expression());
        }
    } while (match(TokenType::COMMA));
    
    if (match(TokenType::FROM)) {
        if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected table name");
        stmt->table_name = current().value;
        advance();
    }
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
    }
    
    if (match(TokenType::ORDER)) {
        if (!match(TokenType::BY)) throw SQLParseException("Expected BY after ORDER");
        
        do {
            if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected column name");
            std::string col = current().value;
            advance();
            
            bool is_desc = false;
            if (match(TokenType::DESC)) {
                is_desc = true;
            } else {
                match(TokenType::ASC);  // Optional ASC
            }
            
            stmt->order_by.push_back({col, is_desc});
        } while (match(TokenType::COMMA));
    }
    
    if (match(TokenType::LIMIT)) {
        if (!check(TokenType::NUMBER)) throw SQLParseException("Expected number after LIMIT");
        stmt->limit = std::stoi(current().value);
        advance();
    }
    
    if (match(TokenType::OFFSET)) {
        if (!check(TokenType::NUMBER)) throw SQLParseException("Expected number after OFFSET");
        stmt->offset = std::stoi(current().value);
        advance();
    }
    
    return stmt;
}

std::shared_ptr<InsertStmt> SQLParser::parse_insert() {
    auto stmt = std::make_shared<InsertStmt>();
    
    if (!match(TokenType::INSERT)) throw SQLParseException("Expected INSERT");
    if (!match(TokenType::INTO)) throw SQLParseException("Expected INTO");
    
    if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected table name");
    stmt->table_name = current().value;
    advance();
    
    if (match(TokenType::LPAREN)) {
        do {
            if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected column name");
            stmt->columns.push_back(current().value);
            advance();
        } while (match(TokenType::COMMA));
        
        if (!match(TokenType::RPAREN)) throw SQLParseException("Expected )");
    }
    
    if (!match(TokenType::VALUES)) throw SQLParseException("Expected VALUES");
    
    do {
        if (!match(TokenType::LPAREN)) throw SQLParseException("Expected (");
        
        std::vector<std::shared_ptr<Expression>> values;
        do {
            values.push_back(parse_expression());
        } while (match(TokenType::COMMA));
        
        if (!match(TokenType::RPAREN)) throw SQLParseException("Expected )");
        
        stmt->values.push_back(values);
    } while (match(TokenType::COMMA));
    
    return stmt;
}

std::shared_ptr<UpdateStmt> SQLParser::parse_update() {
    auto stmt = std::make_shared<UpdateStmt>();
    
    if (!match(TokenType::UPDATE)) throw SQLParseException("Expected UPDATE");
    
    if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected table name");
    stmt->table_name = current().value;
    advance();
    
    if (!match(TokenType::SET)) throw SQLParseException("Expected SET");
    
    do {
        if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected column name");
        std::string col = current().value;
        advance();
        
        if (!match(TokenType::EQUALS)) throw SQLParseException("Expected =");
        
        auto expr = parse_expression();
        stmt->assignments.push_back({col, expr});
    } while (match(TokenType::COMMA));
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
    }
    
    return stmt;
}

std::shared_ptr<DeleteStmt> SQLParser::parse_delete() {
    auto stmt = std::make_shared<DeleteStmt>();
    
    if (!match(TokenType::DELETE)) throw SQLParseException("Expected DELETE");
    if (!match(TokenType::FROM)) throw SQLParseException("Expected FROM");
    
    if (!check(TokenType::IDENTIFIER)) throw SQLParseException("Expected table name");
    stmt->table_name = current().value;
    advance();
    
    if (match(TokenType::WHERE)) {
        stmt->where_clause = parse_expression();
    }
    
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
    auto expr = parse_primary();
    
    while (check(TokenType::MULTIPLY) || check(TokenType::DIVIDE) || check(TokenType::MODULO)) {
        TokenType op = current().type;
        advance();
        auto right = parse_primary();
        expr = std::make_shared<BinaryOp>(expr, right, op);
    }
    
    return expr;
}

std::shared_ptr<Expression> SQLParser::parse_primary() {
    if (match(TokenType::LPAREN)) {
        auto expr = parse_expression();
        if (!match(TokenType::RPAREN)) throw SQLParseException("Expected )");
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
    
    if (check(TokenType::IDENTIFIER)) {
        auto id = std::make_shared<Identifier>(current().value);
        advance();
        return id;
    }
    
    throw SQLParseException("Unexpected token in expression");
}

} // namespace dbx4

