#include "unity.h"
#include "parser.h"
#include "lexer.h"
#include "ast.h"
#include <string.h>

// Helper function to parse an expression
ast_node* parse_expression_helper(const char* source) {
    lexer_t lexer;
    parser_t parser;
    
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    // parser_init already advances to first token, don't advance again
    
    return parse_expression(&parser);
}

// Test parsing numbers
void test_parser_numbers(void) {
    ast_node* node;
    
    node = parse_expression_helper("42");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_NUMBER, node->type);
    ast_free(node);
    
    node = parse_expression_helper("3.14");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_NUMBER, node->type);
    ast_free(node);
}

// Test parsing strings
void test_parser_strings(void) {
    ast_node* node;
    
    node = parse_expression_helper("\"hello\"");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_STRING, node->type);
    ast_free(node);
}

// Test parsing binary expressions
void test_parser_binary_expressions(void) {
    ast_node* node;
    
    // Test addition
    node = parse_expression_helper("1 + 2");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BINARY_OP, node->type);
    ast_free(node);
    
    // Test multiplication
    node = parse_expression_helper("3 * 4");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BINARY_OP, node->type);
    ast_free(node);
    
    // Test precedence
    node = parse_expression_helper("2 + 3 * 4");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BINARY_OP, node->type);
    ast_free(node);
}

// Test parsing unary expressions
void test_parser_unary_expressions(void) {
    ast_node* node;
    
    // Test negation
    node = parse_expression_helper("-42");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_UNARY_OP, node->type);
    ast_free(node);
    
    // Test logical not
    node = parse_expression_helper("!true");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_UNARY_OP, node->type);
    ast_free(node);
}

// Test parsing boolean literals
void test_parser_booleans(void) {
    ast_node* node;
    
    node = parse_expression_helper("true");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BOOLEAN, node->type);
    ast_free(node);
    
    node = parse_expression_helper("false");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BOOLEAN, node->type);
    ast_free(node);
}

// Test parsing null literal
void test_parser_null(void) {
    ast_node* node;
    
    node = parse_expression_helper("null");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_NULL, node->type);
    ast_free(node);
}

// Test parsing parenthesized expressions
void test_parser_parentheses(void) {
    ast_node* node;
    
    // Test grouping changes precedence
    node = parse_expression_helper("(2 + 3) * 4");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_BINARY_OP, node->type);
    ast_free(node);
}

// Test suite runner
void test_parser_suite(void) {
    RUN_TEST(test_parser_numbers);
    RUN_TEST(test_parser_strings);
    RUN_TEST(test_parser_binary_expressions);
    RUN_TEST(test_parser_unary_expressions);
    RUN_TEST(test_parser_booleans);
    RUN_TEST(test_parser_null);
    RUN_TEST(test_parser_parentheses);
}