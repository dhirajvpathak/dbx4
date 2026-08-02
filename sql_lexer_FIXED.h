#pragma once
#include <string>
#include <vector>
#include <unordered_map>
#include <memory>

namespace dbx4 {

enum class TokenType {
    // Keywords
    SELECT, FROM, WHERE, INSERT, INTO, VALUES, UPDATE, SET, DELETE,
    CREATE, TABLE, DROP, ALTER, ADD, MODIFY, PRIMARY, KEY, NOT_KEYWORD, NULL_KEYWORD,
    UNIQUE, FOREIGN, REFERENCES, DEFAULT, CHECK, CONSTRAINT,
    AND, OR, NOT_OP, IN, BETWEEN, LIKE, IS,
    ORDER, BY, GROUP, HAVING, LIMIT, OFFSET, DISTINCT,
    INNER, LEFT, RIGHT, FULL, OUTER, JOIN, ON, USING,
    UNION, INTERSECT, EXCEPT,
    CASE, WHEN, THEN, ELSE, END,
    AS, ASC, DESC,
    
    // Types
    INT, BIGINT, FLOAT, DOUBLE, DECIMAL, VARCHAR, CHAR, BOOLEAN, DATE, TIMESTAMP,
    TEXT, BLOB,
    
    // Operators
    EQUALS, NOT_EQUALS, LESS, LESS_EQUAL, GREATER, GREATER_EQUAL,
    PLUS, MINUS, MULTIPLY, DIVIDE, MODULO,
    CONCAT,
    
    // Delimiters
    LPAREN, RPAREN, COMMA, DOT, SEMICOLON, STAR,
    
    // Literals
    NUMBER, STRING, IDENTIFIER,
    
    // Special
    EOF_TOKEN, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
    
    Token(TokenType t, const std::string& v, int l, int c)
        : type(t), value(v), line(l), column(c) {}
};

class SQLLexer {
private:
    std::string input_;
    size_t pos_;
    int line_;
    int column_;
    
    static const std::unordered_map<std::string, TokenType> keywords_;
    
public:
    SQLLexer(const std::string& input)
        : input_(input), pos_(0), line_(1), column_(1) {}
    
    std::vector<Token> tokenize();
    
private:
    char current();
    char peek();
    void advance();
    void skip_whitespace();
    void skip_comment();
    Token read_string(char quote_char);
    Token read_number();
    Token read_identifier();
    Token read_operator();
    Token make_token(TokenType type, const std::string& value);
};

} // namespace dbx4

