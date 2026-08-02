#pragma once
#include "sql_lexer.h"
#include "../include/dbx4_exceptions.h"
#include <memory>
#include <vector>
namespace dbx4 {
struct ASTNode { virtual ~ASTNode() = default; };
struct Expression : public ASTNode { virtual ~Expression() = default; };
struct Literal : public Expression { std::string value; TokenType type; Literal(const std::string& v, TokenType t) : value(v), type(t) {} };
struct Identifier : public Expression { std::string name; Identifier(const std::string& n) : name(n) {} };
struct BinaryOp : public Expression { std::shared_ptr<Expression> left; std::shared_ptr<Expression> right; TokenType op; BinaryOp(std::shared_ptr<Expression> l, std::shared_ptr<Expression> r, TokenType o) : left(l), right(r), op(o) {} };
struct ColumnDef { std::string name; TokenType type; bool not_null = false; bool primary_key = false; bool unique = false; std::string default_value; };
struct CreateTableStmt : public ASTNode { std::string table_name; std::vector<ColumnDef> columns; };
struct SelectStmt : public ASTNode { bool distinct = false; std::vector<std::shared_ptr<Expression>> columns; std::string table_name; std::shared_ptr<Expression> where_clause; std::vector<std::pair<std::string, bool>> order_by; int limit = -1; int offset = 0; };
struct InsertStmt : public ASTNode { std::string table_name; std::vector<std::string> columns; std::vector<std::vector<std::shared_ptr<Expression>>> values; };
struct UpdateStmt : public ASTNode { std::string table_name; std::vector<std::pair<std::string, std::shared_ptr<Expression>>> assignments; std::shared_ptr<Expression> where_clause; };
struct DeleteStmt : public ASTNode { std::string table_name; std::shared_ptr<Expression> where_clause; };
class SQLParser {
private:
    std::vector<Token> tokens_;
    size_t pos_;
public:
    SQLParser(const std::vector<Token>& tokens) : tokens_(tokens), pos_(0) {}
    std::shared_ptr<ASTNode> parse();
private:
    Token current(); Token peek(); void advance(); bool match(TokenType type); bool check(TokenType type); void consume(TokenType type, const std::string& message);
    std::shared_ptr<ASTNode> parse_statement();
    std::shared_ptr<CreateTableStmt> parse_create_table();
    std::shared_ptr<SelectStmt> parse_select();
    std::shared_ptr<InsertStmt> parse_insert();
    std::shared_ptr<UpdateStmt> parse_update();
    std::shared_ptr<DeleteStmt> parse_delete();
    std::shared_ptr<Expression> parse_expression();
    std::shared_ptr<Expression> parse_or_expression();
    std::shared_ptr<Expression> parse_and_expression();
    std::shared_ptr<Expression> parse_comparison();
    std::shared_ptr<Expression> parse_additive();
    std::shared_ptr<Expression> parse_multiplicative();
    std::shared_ptr<Expression> parse_primary();
};
}
