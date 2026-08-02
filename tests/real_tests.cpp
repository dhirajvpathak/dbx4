// Simple real test framework - no external dependencies
#include <iostream>
#include <cassert>
#include <cstring>
#include "../include/sql_lexer.h"
#include "../include/dbx4_exceptions.h"

using namespace dbx4;

int tests_passed = 0;
int tests_failed = 0;

#define TEST(name) void test_##name(); void run_test_##name() { \
    std::cout << "Running: " << #name << "... "; \
    try { test_##name(); std::cout << "PASS\n"; tests_passed++; } \
    catch (const std::exception& e) { std::cout << "FAIL: " << e.what() << "\n"; tests_failed++; } \
} \
void test_##name()

#define ASSERT_EQ(a, b) assert((a) == (b))
#define ASSERT_GT(a, b) assert((a) > (b))
#define ASSERT_NE(a, b) assert((a) != (b))
#define EXPECT_EQ ASSERT_EQ

// ============================================================================
// LEXER TESTS
// ============================================================================

TEST(LexerTokenizeSelect) {
    SQLLexer lexer("SELECT * FROM users");
    auto tokens = lexer.tokenize();
    ASSERT_GT(tokens.size(), 0);
    ASSERT_EQ(tokens[0].type, TokenType::SELECT);
}

TEST(LexerHandleNull) {
    SQLLexer lexer("NULL");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens[0].type, TokenType::NULL_KEYWORD);
}

TEST(LexerParseString) {
    SQLLexer lexer("'hello'");
    auto tokens = lexer.tokenize();
    ASSERT_EQ(tokens[0].type, TokenType::STRING);
    ASSERT_EQ(tokens[0].value, "hello");
}

TEST(LexerRejectUnterminatedString) {
    try {
        SQLLexer lexer("'unterminated");
        lexer.tokenize();
        assert(false); // Should have thrown
    } catch (const SQLParseException&) {
        // Expected
    }
}

int main() {
    std::cout << "\n=== DBX4 PHASE 3.5.2 - REAL TESTS ===\n\n";
    
    run_test_LexerTokenizeSelect();
    run_test_LexerHandleNull();
    run_test_LexerParseString();
    run_test_LexerRejectUnterminatedString();
    
    std::cout << "\n=== RESULTS ===\n";
    std::cout << "Passed: " << tests_passed << "\n";
    std::cout << "Failed: " << tests_failed << "\n";
    
    return tests_failed > 0 ? 1 : 0;
}
