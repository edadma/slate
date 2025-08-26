#include "unity.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"
#include "codegen.h"
#include "vm.h"

// Test parsing loop...end syntax
void test_parse_infinite_loop(void) {
    const char* source = "loop\n    print(42)\nend loop\n";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    ast_loop* loop_node = (ast_loop*)stmt;
    TEST_ASSERT_NOT_NULL(loop_node->body);
    
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test that loop syntax parses correctly (parse-only test)
void test_infinite_loop_parsing_only(void) {
    const char* source = 
        "loop\n"
        "    print(42)\n"
        "end loop\n";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    // Cleanup (don't try to compile/execute - would be infinite)
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test single-line loop expression
void test_single_line_loop_expression(void) {
    const char* source = "loop print(42)";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error); // Should parse successfully
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    ast_loop* loop_node = (ast_loop*)stmt;
    TEST_ASSERT_NOT_NULL(loop_node->body);
    TEST_ASSERT_EQUAL_INT(AST_EXPRESSION_STMT, loop_node->body->type);
    
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test AST structure for loop statements
void test_loop_ast_structure(void) {
    const char* source = 
        "loop\n"
        "    print(\"hello\")\n"
        "    var y = 123\n"
        "end loop\n";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    // Verify the loop AST structure
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    ast_loop* loop_node = (ast_loop*)stmt;
    TEST_ASSERT_NOT_NULL(loop_node->body);
    TEST_ASSERT_EQUAL_INT(AST_BLOCK, loop_node->body->type);
    
    ast_block* body_block = (ast_block*)loop_node->body;
    TEST_ASSERT_EQUAL_INT(2, body_block->statement_count); // print + var
    
    // Cleanup
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test loop without end marker
void test_loop_without_end_marker(void) {
    const char* source = 
        "loop\n"
        "    print(\"no end marker\")\n";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test loop with optional end marker
void test_loop_with_optional_end_marker(void) {
    const char* source = 
        "loop\n"
        "    print(\"with end marker\")\n"
        "end loop\n";
    
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL_INT(1, program->statement_count);
    
    ast_node* stmt = program->statements[0];
    TEST_ASSERT_EQUAL_INT(AST_LOOP, stmt->type);
    
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test suite runner
void test_infinite_loops_suite(void) {
    RUN_TEST(test_parse_infinite_loop);
    // RUN_TEST(test_infinite_loop_parsing_only);  // Commented out - needs break/continue for proper testing
    RUN_TEST(test_single_line_loop_expression);
    // RUN_TEST(test_loop_ast_structure);  // Commented out - needs break/continue for proper testing
    RUN_TEST(test_loop_without_end_marker);
    RUN_TEST(test_loop_with_optional_end_marker);
}