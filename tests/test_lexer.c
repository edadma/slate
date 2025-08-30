#include "unity.h"
#include "lexer.h"
#include <string.h>
#include <stdlib.h>

// Test lexing numbers
void test_lexer_numbers(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "42 3.14 0 1e5");
    
    // Test integer (should be TOKEN_INTEGER now)
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_INTEGER, token.type);
    
    // Test float (should be TOKEN_NUMBER)
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NUMBER, token.type);
    
    // Test zero integer (should be TOKEN_INTEGER)
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_INTEGER, token.type);
    
    // Test scientific notation (should be TOKEN_NUMBER)
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NUMBER, token.type);
    
    // Test EOF
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_EOF, token.type);
}

// Test lexing strings
void test_lexer_strings(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "\"hello\" \"world\" \"\" 'single' 'quotes' ''");
    
    // Test first double-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test second double-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test empty double-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test first single-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test second single-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test empty single-quoted string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
}

// Test lexing operators
void test_lexer_operators(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "+ - * / == != < <= > >=");
    
    // Test arithmetic operators
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_PLUS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MINUS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MULTIPLY, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_DIVIDE, token.type);
    
    // Test comparison operators
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NOT_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LESS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LESS_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_GREATER, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_GREATER_EQUAL, token.type);
}

// Test lexing keywords
void test_lexer_keywords(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "if else while do return function var true false null undefined mod");
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IF, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_ELSE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_WHILE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_DO, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_RETURN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_FUNCTION, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_VAR, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_TRUE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_FALSE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NULL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_UNDEFINED, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MOD, token.type);
}

// Test lexing identifiers
void test_lexer_identifiers(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "foo bar_baz");
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
}

// Test lexing compound assignment operators
void test_lexer_compound_assignments(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "+= -= *= /= %= **= &= |= ^= &&= ||=");
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_PLUS_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MINUS_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MULT_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_DIV_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MOD_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_POWER_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_BITWISE_AND_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_BITWISE_OR_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_BITWISE_XOR_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LOGICAL_AND_ASSIGN, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LOGICAL_OR_ASSIGN, token.type);
    
    lexer_cleanup(&lexer);
}

// Test single-quoted strings with escape sequences
void test_lexer_single_quote_escapes(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "'hello\\nworld' 'tab\\there' 'quote\\'' \"double\\\"quote\"");
    
    // Test newline escape in single quotes
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    TEST_ASSERT_EQUAL_INT(14, token.length); // 'hello\nworld' (includes quotes)
    
    // Test tab escape in single quotes
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test single quote escape in single quotes
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    // Test double quote escape in double quotes
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    
    lexer_cleanup(&lexer);
}

// Test suite runner
void test_lexer_suite(void) {
    RUN_TEST(test_lexer_numbers);
    RUN_TEST(test_lexer_strings);
    RUN_TEST(test_lexer_single_quote_escapes);
    RUN_TEST(test_lexer_operators);
    RUN_TEST(test_lexer_keywords);
    RUN_TEST(test_lexer_identifiers);
    RUN_TEST(test_lexer_compound_assignments);
}