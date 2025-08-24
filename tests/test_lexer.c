#include "unity.h"
#include "lexer.h"
#include <string.h>

// Test lexing numbers
void test_lexer_numbers(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "42 3.14 0");
    
    // Test integer
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, token.value.number);
    
    // Test float
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL_DOUBLE(3.14, token.value.number);
    
    // Test zero
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_NUMBER, token.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, token.value.number);
    
    // Test EOF
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_EOF, token.type);
}

// Test lexing strings
void test_lexer_strings(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "\"hello\" \"world\" \"\"");
    
    // Test first string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    TEST_ASSERT_EQUAL_STRING("hello", token.value.string);
    
    // Test second string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    TEST_ASSERT_EQUAL_STRING("world", token.value.string);
    
    // Test empty string
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STRING, token.type);
    TEST_ASSERT_EQUAL_STRING("", token.value.string);
    
    // Cleanup allocated strings
    free(token.value.string);
}

// Test lexing operators
void test_lexer_operators(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "+ - * / == != < <= > >= && || !");
    
    // Test arithmetic operators
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_PLUS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_MINUS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_STAR, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_SLASH, token.type);
    
    // Test comparison operators
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_EQUAL_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_BANG_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LESS, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_LESS_EQUAL, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_GREATER, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_GREATER_EQUAL, token.type);
    
    // Test logical operators
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_AND_AND, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_OR_OR, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_BANG, token.type);
}

// Test lexing keywords
void test_lexer_keywords(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "if else while for return function var true false null");
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IF, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_ELSE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_WHILE, token.type);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_FOR, token.type);
    
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
}

// Test lexing identifiers
void test_lexer_identifiers(void) {
    lexer_t lexer;
    token_t token;
    
    lexer_init(&lexer, "foo bar_baz camelCase PascalCase _underscore x123");
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("foo", token.value.string);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("bar_baz", token.value.string);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("camelCase", token.value.string);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("PascalCase", token.value.string);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("_underscore", token.value.string);
    
    token = lexer_next_token(&lexer);
    TEST_ASSERT_EQUAL_INT(TOKEN_IDENTIFIER, token.type);
    TEST_ASSERT_EQUAL_STRING("x123", token.value.string);
}

// Test suite runner
void test_lexer_suite(void) {
    RUN_TEST(test_lexer_numbers);
    RUN_TEST(test_lexer_strings);
    RUN_TEST(test_lexer_operators);
    RUN_TEST(test_lexer_keywords);
    RUN_TEST(test_lexer_identifiers);
}