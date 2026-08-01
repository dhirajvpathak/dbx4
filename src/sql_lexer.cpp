#include "sql_lexer.h"
#include <cctype>
#include <algorithm>

namespace dbx4 {

const std::unordered_map<std::string, TokenType> SQLLexer::keywords_ = {
    // SQL Keywords
    {"SELECT", TokenType::SELECT},
    {"FROM", TokenType::FROM},
    {"WHERE", TokenType::WHERE},
    {"INSERT", TokenType::INSERT},
    {"INTO", TokenType::INTO},
    {"VALUES", TokenType::VALUES},
    {"UPDATE", TokenType::UPDATE},
    {"SET", TokenType::SET},
    {"DELETE", TokenType::DELETE},
    {"CREATE", TokenType::CREATE},
    {"TABLE", TokenType::TABLE},
    {"DROP", TokenType::DROP},
    {"ALTER", TokenType::ALTER},
    {"ADD", TokenType::ADD},
    {"MODIFY", TokenType::MODIFY},
    {"PRIMARY", TokenType::PRIMARY},
    {"KEY", TokenType::KEY},
    {"NOT", TokenType::NOT},
    {"NULL", TokenType::NULL},
    {"UNIQUE", TokenType::UNIQUE},
    {"FOREIGN", TokenType::FOREIGN},
    {"REFERENCES", TokenType::REFERENCES},
    {"DEFAULT", TokenType::DEFAULT},
    {"CHECK", TokenType::CHECK},
    {"CONSTRAINT", TokenType::CONSTRAINT},
    {"AND", TokenType::AND},
    {"OR", TokenType::OR},
    {"IN", TokenType::IN},
    {"BETWEEN", TokenType::BETWEEN},
    {"LIKE", TokenType::LIKE},
    {"IS", TokenType::IS},
    {"ORDER", TokenType::ORDER},
    {"BY", TokenType::BY},
    {"GROUP", TokenType::GROUP},
    {"HAVING", TokenType::HAVING},
    {"LIMIT", TokenType::LIMIT},
    {"OFFSET", TokenType::OFFSET},
    {"DISTINCT", TokenType::DISTINCT},
    {"INNER", TokenType::INNER},
    {"LEFT", TokenType::LEFT},
    {"RIGHT", TokenType::RIGHT},
    {"FULL", TokenType::FULL},
    {"OUTER", TokenType::OUTER},
    {"JOIN", TokenType::JOIN},
    {"ON", TokenType::ON},
    {"USING", TokenType::USING},
    {"UNION", TokenType::UNION},
    {"INTERSECT", TokenType::INTERSECT},
    {"EXCEPT", TokenType::EXCEPT},
    {"CASE", TokenType::CASE},
    {"WHEN", TokenType::WHEN},
    {"THEN", TokenType::THEN},
    {"ELSE", TokenType::ELSE},
    {"END", TokenType::END},
    {"AS", TokenType::AS},
    {"ASC", TokenType::ASC},
    {"DESC", TokenType::DESC},
    
    // Data types
    {"INT", TokenType::INT},
    {"BIGINT", TokenType::BIGINT},
    {"FLOAT", TokenType::FLOAT},
    {"DOUBLE", TokenType::DOUBLE},
    {"DECIMAL", TokenType::DECIMAL},
    {"VARCHAR", TokenType::VARCHAR},
    {"CHAR", TokenType::CHAR},
    {"BOOLEAN", TokenType::BOOLEAN},
    {"DATE", TokenType::DATE},
    {"TIMESTAMP", TokenType::TIMESTAMP},
    {"TEXT", TokenType::TEXT},
    {"BLOB", TokenType::BLOB},
};

char SQLLexer::current() {
    if (pos_ >= input_.size()) return '\0';
    return input_[pos_];
}

char SQLLexer::peek() {
    if (pos_ + 1 >= input_.size()) return '\0';
    return input_[pos_ + 1];
}

void SQLLexer::advance() {
    if (pos_ < input_.size()) {
        if (input_[pos_] == '\n') {
            line_++;
            column_ = 1;
        } else {
            column_++;
        }
        pos_++;
    }
}

void SQLLexer::skip_whitespace() {
    while (std::isspace(current())) {
        advance();
    }
}

void SQLLexer::skip_comment() {
    if (current() == '-' && peek() == '-') {
        while (current() != '\n' && current() != '\0') {
            advance();
        }
        if (current() == '\n') advance();
    } else if (current() == '/' && peek() == '*') {
        advance();
        advance();
        while (current() != '\0') {
            if (current() == '*' && peek() == '/') {
                advance();
                advance();
                break;
            }
            advance();
        }
    }
}

Token SQLLexer::read_string() {
    int start_line = line_;
    int start_col = column_;
    char quote = current();
    advance();
    
    std::string value;
    while (current() != quote && current() != '\0') {
        if (current() == '\\') {
            advance();
            switch (current()) {
                case 'n': value += '\n'; break;
                case 't': value += '\t'; break;
                case 'r': value += '\r'; break;
                case '\\': value += '\\'; break;
                case '"': value += '"'; break;
                case '\'': value += '\''; break;
                default: value += current();
            }
        } else {
            value += current();
        }
        advance();
    }
    
    if (current() == quote) advance();
    return Token(TokenType::STRING, value, start_line, start_col);
}

Token SQLLexer::read_number() {
    int start_line = line_;
    int start_col = column_;
    
    std::string value;
    bool has_dot = false;
    
    while (std::isdigit(current()) || (current() == '.' && !has_dot)) {
        if (current() == '.') has_dot = true;
        value += current();
        advance();
    }
    
    return Token(TokenType::NUMBER, value, start_line, start_col);
}

Token SQLLexer::read_identifier() {
    int start_line = line_;
    int start_col = column_;
    
    std::string value;
    while (std::isalnum(current()) || current() == '_') {
        value += current();
        advance();
    }
    
    // Convert to uppercase for keyword matching
    std::string upper_value = value;
    std::transform(upper_value.begin(), upper_value.end(), upper_value.begin(), ::toupper);
    
    auto it = keywords_.find(upper_value);
    if (it != keywords_.end()) {
        return Token(it->second, value, start_line, start_col);
    }
    
    return Token(TokenType::IDENTIFIER, value, start_line, start_col);
}

Token SQLLexer::read_operator() {
    int start_line = line_;
    int start_col = column_;
    
    char ch = current();
    advance();
    
    switch (ch) {
        case '=': return Token(TokenType::EQUALS, "=", start_line, start_col);
        case '<':
            if (current() == '=') { advance(); return Token(TokenType::LESS_EQUAL, "<=", start_line, start_col); }
            if (current() == '>') { advance(); return Token(TokenType::NOT_EQUALS, "<>", start_line, start_col); }
            return Token(TokenType::LESS, "<", start_line, start_col);
        case '>':
            if (current() == '=') { advance(); return Token(TokenType::GREATER_EQUAL, ">=", start_line, start_col); }
            return Token(TokenType::GREATER, ">", start_line, start_col);
        case '!':
            if (current() == '=') { advance(); return Token(TokenType::NOT_EQUALS, "!=", start_line, start_col); }
            return Token(TokenType::UNKNOWN, "!", start_line, start_col);
        case '+': return Token(TokenType::PLUS, "+", start_line, start_col);
        case '-': return Token(TokenType::MINUS, "-", start_line, start_col);
        case '*': return Token(TokenType::MULTIPLY, "*", start_line, start_col);
        case '/': return Token(TokenType::DIVIDE, "/", start_line, start_col);
        case '%': return Token(TokenType::MODULO, "%", start_line, start_col);
        case '|':
            if (current() == '|') { advance(); return Token(TokenType::CONCAT, "||", start_line, start_col); }
            return Token(TokenType::UNKNOWN, "|", start_line, start_col);
        default: return Token(TokenType::UNKNOWN, std::string(1, ch), start_line, start_col);
    }
}

std::vector<Token> SQLLexer::tokenize() {
    std::vector<Token> tokens;
    
    while (pos_ < input_.size()) {
        skip_whitespace();
        
        if (current() == '\0') break;
        
        // Comments
        if ((current() == '-' && peek() == '-') || (current() == '/' && peek() == '*')) {
            skip_comment();
            continue;
        }
        
        // Strings
        if (current() == '\'' || current() == '"') {
            tokens.push_back(read_string());
            continue;
        }
        
        // Numbers
        if (std::isdigit(current())) {
            tokens.push_back(read_number());
            continue;
        }
        
        // Identifiers and keywords
        if (std::isalpha(current()) || current() == '_') {
            tokens.push_back(read_identifier());
            continue;
        }
        
        // Delimiters
        switch (current()) {
            case '(': tokens.push_back(Token(TokenType::LPAREN, "(", line_, column_)); advance(); break;
            case ')': tokens.push_back(Token(TokenType::RPAREN, ")", line_, column_)); advance(); break;
            case ',': tokens.push_back(Token(TokenType::COMMA, ",", line_, column_)); advance(); break;
            case '.': tokens.push_back(Token(TokenType::DOT, ".", line_, column_)); advance(); break;
            case ';': tokens.push_back(Token(TokenType::SEMICOLON, ";", line_, column_)); advance(); break;
            case '*': tokens.push_back(Token(TokenType::STAR, "*", line_, column_)); advance(); break;
            default:
                tokens.push_back(read_operator());
        }
    }
    
    tokens.push_back(Token(TokenType::EOF_TOKEN, "", line_, column_));
    return tokens;
}

} // namespace dbx4

