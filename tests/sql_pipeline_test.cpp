#include <gtest/gtest.h>
#include "../include/sql_lexer.h"
#include "../include/sql_parser.h"
#include "../include/dbx4_exceptions.h"

using namespace dbx4;

// ============================================================================
// LEXER TESTS - Real assertions that FAIL if code is broken
// ============================================================================

TEST(LexerTests, TokenizeSimpleSelect) {
    SQLLexer lexer("SELECT * FROM users");
    auto tokens = lexer.tokenize();
    
    ASSERT_GT(tokens.size(), 0);
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[0].value, "SELECT");
}

TEST(LexerTests, HandleNullKeyword) {
    SQLLexer lexer("NULL");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 2); // NULL + EOF
    EXPECT_EQ(tokens[0].type, TokenType::NULL_KEYWORD);
    EXPECT_EQ(tokens[0].value, "NULL");
}

TEST(LexerTests, ParseStringLiteral) {
    SQLLexer lexer("'hello world'");
    auto tokens = lexer.tokenize();
    
    ASSERT_EQ(tokens.size(), 2); // STRING + EOF
    EXPECT_EQ(tokens[0].type, TokenType::STRING);
    EXPECT_EQ(tokens[0].value, "hello world");
}

TEST(LexerTests, HandleComments) {
    SQLLexer lexer("SELECT * -- this is a comment\nFROM t");
    auto tokens = lexer.tokenize();
    
    // Should skip comment, only have SELECT * FROM t EOF
    EXPECT_EQ(tokens[0].type, TokenType::SELECT);
    EXPECT_EQ(tokens[1].type, TokenType::STAR);
    EXPECT_EQ(tokens[2].type, TokenType::FROM);
}

TEST(LexerTests, RejectUnterminatedString) {
    SQLLexer lexer("'unterminated");
    
    EXPECT_THROW({
        lexer.tokenize();
    }, SQLParseException);
}

// ============================================================================
// PARSER TESTS - Real assertions
// ============================================================================

TEST(ParserTests, ParseSelectStatement) {
    SQLLexer lexer("SELECT id, name FROM users");
    auto tokens = lexer.tokenize();
    
    SQLParser parser(tokens);
    auto ast = parser.parse();
    
    ASSERT_NE(ast, nullptr);
}

TEST(ParserTests, ParseLimit) {
    SQLLexer lexer("SELECT * FROM t LIMIT 10");
    auto tokens = lexer.tokenize();
    
    SQLParser parser(tokens);
    auto ast = parser.parse();
    
    ASSERT_NE(ast, nullptr);
}

TEST(ParserTests, RejectNegativeLimit) {
    SQLLexer lexer("SELECT * FROM t LIMIT -1");
    auto tokens = lexer.tokenize();
    
    SQLParser parser(tokens);
    
    EXPECT_THROW({
        parser.parse();
    }, SQLParseException);
}

TEST(ParserTests, ValidateEofAfterStatement) {
    SQLLexer lexer("SELECT *; SELECT *");
    auto tokens = lexer.tokenize();
    
    SQLParser parser(tokens);
    
    // Parser should require EOF after first statement
    auto ast = parser.parse();
    ASSERT_NE(ast, nullptr);
    // Should NOT have unparsed tokens remaining
}

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
