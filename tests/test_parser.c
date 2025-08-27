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
    
    ast_node* result = parse_expression(&parser);
    lexer_cleanup(&lexer);
    return result;
}

// Test parsing numbers
void test_parser_numbers(void) {
    ast_node* node;
    
    // Integer literal should parse as AST_INTEGER
    node = parse_expression_helper("42");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_INTEGER, node->type);
    ast_free(node);
    
    // Float literal should parse as AST_NUMBER
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

// Test parsing undefined literal
void test_parser_undefined(void) {
    ast_node* node;
    
    node = parse_expression_helper("undefined");
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_EQUAL_INT(AST_UNDEFINED, node->type);
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

// Test undefined parsing (should parse successfully, runtime will handle restrictions)
void test_parser_undefined_assignment_restrictions(void) {
    lexer_t lexer;
    parser_t parser;
    ast_node* node;
    
    // Test variable declaration with undefined should parse successfully
    lexer_init(&lexer, "var x = undefined");
    parser_init(&parser, &lexer);
    node = parse_declaration(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_VAR_DECLARATION, node->type);
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test assignment with undefined should parse successfully
    lexer_init(&lexer, "x = undefined");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_ASSIGNMENT, node->type);
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test return with undefined should parse successfully
    lexer_init(&lexer, "return undefined");
    parser_init(&parser, &lexer);
    node = parse_statement(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_RETURN, node->type);
    ast_free(node);
    lexer_cleanup(&lexer);
}

// Test parsing compound assignments
void test_parser_compound_assignments(void) {
    ast_node* node;
    lexer_t lexer;
    parser_t parser;
    
    // Test += operator
    lexer_init(&lexer, "x += 5");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    ast_compound_assignment* comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_ADD, comp_assign->op);
    TEST_ASSERT_NOT_NULL(comp_assign->target);
    TEST_ASSERT_EQUAL_INT(AST_IDENTIFIER, comp_assign->target->type);
    TEST_ASSERT_NOT_NULL(comp_assign->value);
    TEST_ASSERT_EQUAL_INT(AST_INTEGER, comp_assign->value->type);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test -= operator
    lexer_init(&lexer, "y -= 3");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_SUBTRACT, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test *= operator
    lexer_init(&lexer, "z *= 2");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_MULTIPLY, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test /= operator
    lexer_init(&lexer, "w /= 4");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_DIVIDE, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test %= operator
    lexer_init(&lexer, "m %= 3");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_MOD, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test **= operator
    lexer_init(&lexer, "p **= 2");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_POWER, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
}

// Test new compound assignment operators
void test_parser_new_compound_assignments(void) {
    ast_node* node;
    lexer_t lexer;
    parser_t parser;
    ast_compound_assignment* comp_assign;
    
    // Test &= operator  
    lexer_init(&lexer, "x &= 5");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_BITWISE_AND, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test |= operator
    lexer_init(&lexer, "y |= 3");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_BITWISE_OR, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test ^= operator
    lexer_init(&lexer, "z ^= 7");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_BITWISE_XOR, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test &&= operator
    lexer_init(&lexer, "a &&= true");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_LOGICAL_AND, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
    
    // Test ||= operator
    lexer_init(&lexer, "b ||= false");
    parser_init(&parser, &lexer);
    node = parse_expression(&parser);
    TEST_ASSERT_NOT_NULL(node);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(AST_COMPOUND_ASSIGNMENT, node->type);
    
    comp_assign = (ast_compound_assignment*)node;
    TEST_ASSERT_EQUAL_INT(BIN_LOGICAL_OR, comp_assign->op);
    
    ast_free(node);
    lexer_cleanup(&lexer);
}

// Test suite runner
void test_parser_suite(void) {
    RUN_TEST(test_parser_numbers);
    RUN_TEST(test_parser_strings);
    RUN_TEST(test_parser_binary_expressions);
    RUN_TEST(test_parser_unary_expressions);
    RUN_TEST(test_parser_booleans);
    RUN_TEST(test_parser_null);
    RUN_TEST(test_parser_undefined);
    RUN_TEST(test_parser_parentheses);
    RUN_TEST(test_parser_undefined_assignment_restrictions);
    RUN_TEST(test_parser_compound_assignments);
    RUN_TEST(test_parser_new_compound_assignments);
}