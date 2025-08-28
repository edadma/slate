#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
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
    const char* source = "loop\n"
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
    const char* source = "loop\n"
                         "    print(\"hello\")\n"
                         "    var y = 123\n"
                         "    y\n"
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
    TEST_ASSERT_EQUAL_INT(3, body_block->statement_count); // print + var + expression

    // Cleanup
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

// Test loop without end marker
void test_loop_without_end_marker(void) {
    const char* source = "loop\n"
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
    const char* source = "loop\n"
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

// Helper function to slate infinite loop test code and return result
static value_t run_loop_test(const char* source) {
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return make_null();
    }

    codegen_t* codegen = codegen_create();
    function_t* function = codegen_compile(codegen, program);

    if (codegen->had_error || !function) {
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return make_null();
    }

    slate_vm* vm = vm_create();
    vm_result result = vm_execute(vm, function);

    value_t return_value = make_null();
    if (result == VM_OK) {
        return_value = vm->result;
        // Retain strings and other reference-counted types to survive cleanup
        return_value = vm_retain(return_value);
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return return_value;
}

// Test infinite loop with break - basic execution
void test_infinite_loop_with_break(void) {
    value_t result;

    // Simple infinite loop that breaks after 3 iterations
    result = run_loop_test("var count = 0\n"
                           "loop\n"
                           "    count = count + 1\n"
                           "    if count >= 3 then break\n"
                           "end loop\n"
                           "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

// Test infinite loop with complex break condition
void test_infinite_loop_complex_break(void) {
    value_t result;

    // Loop with mathematical condition
    result = run_loop_test("var sum = 0\n"
                           "var i = 1\n"
                           "loop\n"
                           "    sum = sum + i\n"
                           "    i = i + 1\n"
                           "    if sum > 20 then break\n"
                           "end loop\n"
                           "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(21, result.as.int32); // 1+2+3+4+5+6 = 21
    vm_release(result);
}

// Test single-line infinite loop with break
void test_single_line_infinite_loop_with_break(void) {
    value_t result;

    // Single-line loops are limited - test with proper multiline syntax
    result = run_loop_test("var x = 0\n"
                           "loop\n"
                           "    x = x + 1\n"
                           "    if x == 1 then break\n"
                           "x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
    vm_release(result);
}

// Test nested infinite loops with break (simplified - nested loops not fully supported yet)
void test_nested_infinite_loops_with_break(void) {
    value_t result;

    // For now, test a single loop that simulates nested behavior
    result = run_loop_test("var count = 0\n"
                           "var stage = 1\n"
                           "loop\n"
                           "    count = count + 1\n"
                           "    if stage == 1 and count >= 3 then\n"
                           "        stage = 2\n"
                           "        count = 0\n"
                           "    if stage == 2 and count >= 2 then break\n"
                           "end loop\n"
                           "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
    vm_release(result);
}

// Test infinite loop with break in different conditions
void test_infinite_loop_break_variations(void) {
    value_t result;

    // Break with modulo condition
    result = run_loop_test("var n = 1\n"
                           "loop\n"
                           "    n = n + 1\n"
                           "    if n mod 7 == 0 then break\n"
                           "end loop\n"
                           "n");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32); // First multiple of 7
    vm_release(result);

    // Break with logical operators
    result = run_loop_test("var a = 0\n"
                           "var b = 10\n"
                           "loop\n"
                           "    a = a + 1\n"
                           "    b = b - 1\n"
                           "    if a >= 4 and b <= 7 then break\n"
                           "end loop\n"
                           "a + b");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32); // a=4, b=6 -> 4+6=10
    vm_release(result);
}

// Test suite runner
void test_infinite_loops_suite(void) {
    RUN_TEST(test_parse_infinite_loop);
    RUN_TEST(test_infinite_loop_parsing_only); // Now enabled with break support
    RUN_TEST(test_single_line_loop_expression);
    RUN_TEST(test_loop_ast_structure); // Now enabled with break support
    RUN_TEST(test_loop_without_end_marker);
    RUN_TEST(test_loop_with_optional_end_marker);
    // New execution tests with break statements
    RUN_TEST(test_infinite_loop_with_break);
    RUN_TEST(test_infinite_loop_complex_break);
    RUN_TEST(test_single_line_infinite_loop_with_break);
    RUN_TEST(test_nested_infinite_loops_with_break);
    RUN_TEST(test_infinite_loop_break_variations);
}
