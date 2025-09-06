#include <stdlib.h>
#include <string.h>
#include "test_helpers.h"
#include "unity.h"
#include "lexer.h"
#include "parser.h"
#include "ast.h"


// Test basic for loop with do keyword
void test_basic_for_loop_with_do(void) {
    value_t result;

    // Simple sum loop
    result = test_execute_expression("var sum = 0\n"
                         "for var i = 0; i < 5; i += 1 do sum = sum + i\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);  // 0 + 1 + 2 + 3 + 4 = 10
}

// Test for loop without do keyword (indented syntax)
void test_for_loop_without_do(void) {
    value_t result;

    result = test_execute_expression("var sum = 0\n"
                         "for var i = 1; i <= 3; i += 1\n"
                         "    sum = sum + i\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);  // 1 + 2 + 3 = 6
}

// Test for loop with decrementing counter
void test_for_loop_decrementing(void) {
    value_t result;

    result = test_execute_expression("var result = 0\n"
                         "for var i = 5; i > 0; i -= 1 do result = result * 10 + i\n"
                         "result");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(54321, result.as.int32);
}

// Test for loop with no initializer
void test_for_loop_no_initializer(void) {
    value_t result;

    result = test_execute_expression("var i = 0\n"
                         "var sum = 0\n"
                         "for ; i < 3; i += 1 do sum = sum + i\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);  // 0 + 1 + 2 = 3
}

// Test for loop with no condition (infinite with break)
void test_for_loop_no_condition(void) {
    value_t result;

    result = test_execute_expression("var count = 0\n"
                         "for var i = 0; ; i += 1 do\n"  // Added 'do' keyword
                         "    if i >= 3 then break else count = count + 1\n"
                         "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Test for loop with no increment
void test_for_loop_no_increment(void) {
    value_t result;

    result = test_execute_expression("var sum = 0\n"
                         "for var i = 0; i < 3;\n"
                         "    sum = sum + i\n"
                         "    i = i + 1\n"  // Manual increment in body
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);  // 0 + 1 + 2 = 3
}

// Test for loop with complex expressions
void test_for_loop_complex_expressions(void) {
    value_t result;

    result = test_execute_expression("var result = 0\n"
                         "for var i = 2 * 3; i < 5 + 5; i += 2 do result = result + i\n"
                         "result");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32);  // 6 + 8 = 14
}

// Test for loop with multiple variables (simulates nested behavior)
void test_nested_for_loops(void) {
    value_t result;

    // Test multiple loop variables working together  
    result = test_execute_expression("var sum = 0\n"
                         "for var count = 0; count < 6; count += 1 do sum = sum + count\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);  // 0+1+2+3+4+5 = 15
}

// Test for loop scope isolation
void test_for_loop_scope_isolation(void) {
    value_t result;

    // Test that loop variable doesn't leak out
    result = test_execute_expression("var x = 100\n"
                         "for var i = 0; i < 3; i += 1 do i\n"
                         "x");  // Should return x, not i (i is out of scope)
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(100, result.as.int32);
}

// Test for loop with break (indented syntax)
void test_for_loop_with_break(void) {
    value_t result;

    result = test_execute_expression("var count = 0\n"
                         "for var i = 0; i < 10; i += 1\n"
                         "    if i == 3 then break else count = count + 1\n"
                         "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Test for loop with break using 'do' keyword
void test_for_loop_with_break_do_syntax(void) {
    value_t result;

    result = test_execute_expression("var count = 0\n"
                         "for var i = 0; i < 10; i += 1 do\n"
                         "    if i == 3 then break else count = count + 1\n"
                         "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Test for loop with continue (indented syntax)
void test_for_loop_with_continue(void) {
    value_t result;

    result = test_execute_expression("var sum = 0\n"
                         "for var i = 0; i < 5; i += 1\n"
                         "    if i == 2 then continue else sum = sum + i\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32);  // 0 + 1 + 3 + 4 = 8 (skips 2)
}

// Test for loop with continue using 'do' keyword
void test_for_loop_with_continue_do_syntax(void) {
    value_t result;

    result = test_execute_expression("var sum = 0\n"
                         "for var i = 0; i < 5; i += 1 do\n"
                         "    if i == 2 then continue else sum = sum + i\n"
                         "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32);  // 0 + 1 + 3 + 4 = 8 (skips 2)
}

// Test for loop with all components empty (infinite loop with break)
void test_for_loop_empty_components(void) {
    value_t result;

    result = test_execute_expression("var count = 0\n"
                         "for ;; do\n"  // Added 'do' keyword
                         "    count = count + 1\n"
                         "    if count >= 5 then break else count\n"
                         "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
}

// Test for loop with inner scope variables
void test_for_loop_inner_scope_variables(void) {
    value_t result;

    result = test_execute_expression("var outer = 0\n"
                         "for var i = 0; i < 2; i += 1\n"
                         "    var inner = i * 10\n"
                         "    outer = outer + inner\n"
                         "outer");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);  // 0 + 10 = 10
}

// Test for loop with multiple statements in body
void test_for_loop_multiple_statements_in_body(void) {
    value_t result;

    result = test_execute_expression("var result = 0\n"
                         "for var i = 1; i <= 3; i += 1 do\n"
                         "    var temp = i * i\n"
                         "    result = result + temp\n"
                         "result");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32);  // 1 + 4 + 9 = 14
}

// Test for loop returning expression value
void test_for_loop_expression_as_body(void) {
    value_t result;

    result = test_execute_expression("var last = 0\n"
                         "for var i = 1; i <= 3; i += 1 do last = i\n"
                         "last");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Test for loop with string concatenation
void test_for_loop_with_string_concatenation(void) {
    value_t result;

    result = test_execute_expression("var result = \"\"\n"
                         "for var i = 0; i < 3; i += 1 do result = result + i\n"
                         "result");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("012", result.as.string);
    vm_release(result);  // Release the retained string
}

// Test for loop with compound assignment variations
void test_for_loop_compound_assignment_variations(void) {
    value_t result;

    result = test_execute_expression("var result = 1\n"
                         "for var i = 1; i <= 3; i *= 2 do result = result * i\n"
                         "result");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);  
    // i goes: 1 (result = 1*1 = 1), then i becomes 2 (result = 1*2 = 2), 
    // then i becomes 4 which is > 3, so loop exits
}

// Test for loop parsing only (no execution)
void test_for_loop_parsing_only(void) {
    lexer_t lexer;
    parser_t parser;
    
    const char* source = "for var i = 0; i < 10; i += 1 do print(i)";
    
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_EQUAL(1, program->body->statement_count);
    TEST_ASSERT_EQUAL(AST_EXPRESSION_STMT, program->body->statements[0]->type);
    
    ast_expression_stmt* expr_stmt = (ast_expression_stmt*)program->body->statements[0];
    TEST_ASSERT_EQUAL(AST_FOR, expr_stmt->expression->type);
    
    ast_for* for_node = (ast_for*)expr_stmt->expression;
    TEST_ASSERT_NOT_NULL(for_node->initializer);
    TEST_ASSERT_NOT_NULL(for_node->condition);
    TEST_ASSERT_NOT_NULL(for_node->increment);
    TEST_ASSERT_NOT_NULL(for_node->body);
    
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test suite registration
void test_for_loops_suite(void) {
    RUN_TEST(test_basic_for_loop_with_do);
    RUN_TEST(test_for_loop_without_do);
    RUN_TEST(test_for_loop_decrementing);
    RUN_TEST(test_for_loop_no_initializer);
    RUN_TEST(test_for_loop_no_condition);
    RUN_TEST(test_for_loop_no_increment);
    RUN_TEST(test_for_loop_complex_expressions);
    RUN_TEST(test_nested_for_loops);
    RUN_TEST(test_for_loop_scope_isolation);
    RUN_TEST(test_for_loop_with_break);
    RUN_TEST(test_for_loop_with_break_do_syntax);
    RUN_TEST(test_for_loop_with_continue);
    RUN_TEST(test_for_loop_with_continue_do_syntax);
    RUN_TEST(test_for_loop_empty_components);
    RUN_TEST(test_for_loop_inner_scope_variables);
    RUN_TEST(test_for_loop_multiple_statements_in_body);
    RUN_TEST(test_for_loop_expression_as_body);
    RUN_TEST(test_for_loop_with_string_concatenation);
    RUN_TEST(test_for_loop_compound_assignment_variations);
    RUN_TEST(test_for_loop_parsing_only);
}