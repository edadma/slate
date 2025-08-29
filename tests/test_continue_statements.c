#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to slate continue statement test code and return result
static value_t run_continue_test(const char* source) {
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return make_null();
    }

    slate_vm* vm = vm_create();
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);

    if (codegen->had_error || !function) {
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return make_null();
    }

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

// Test continue in while loop - skip odd numbers
void test_continue_in_while_loop_skip_odd(void) {
    value_t result;

    // Sum only even numbers using continue to skip odd
    result = run_continue_test("var sum = 0\n"
                               "var i = 0\n"
                               "while i < 10 do\n"
                               "    i = i + 1\n"
                               "    if i mod 2 != 0 then continue\n"
                               "    sum = sum + i\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(30, result.as.int32); // 2+4+6+8+10 = 30
    vm_release(result);
}

// Test continue in infinite loop
void test_continue_in_infinite_loop(void) {
    value_t result;

    // Use continue to skip certain iterations
    result = run_continue_test("var count = 0\n"
                               "var sum = 0\n"
                               "loop\n"
                               "    count = count + 1\n"
                               "    if count > 10 then break\n"
                               "    if count mod 3 == 0 then continue\n"
                               "    sum = sum + count\n"
                               "end loop\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(37, result.as.int32); // 1+2+4+5+7+8+10 = 37 (skip 3,6,9)
    vm_release(result);
}

// Test continue with complex conditions
void test_continue_with_complex_conditions(void) {
    value_t result;

    // Continue with multiple conditions
    result = run_continue_test("var count = 0\n"
                               "var processed = 0\n"
                               "while count < 20 do\n"
                               "    count = count + 1\n"
                               "    if count mod 3 == 0 or count mod 5 == 0 then continue\n"
                               "    processed = processed + 1\n"
                               "end while\n"
                               "processed");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(11, result.as.int32); // Numbers not divisible by 3 or 5
    vm_release(result);
}

// Test continue and break together in while loop
void test_continue_and_break_in_while(void) {
    value_t result;

    // Use both continue and break
    result = run_continue_test("var i = 0\n"
                               "var sum = 0\n"
                               "while i < 100 do\n"
                               "    i = i + 1\n"
                               "    if i > 10 then break\n"
                               "    if i == 5 then continue\n"
                               "    sum = sum + i\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(50, result.as.int32); // 1+2+3+4+6+7+8+9+10 = 50 (skip 5)
    vm_release(result);
}

// Test continue and break together in infinite loop
void test_continue_and_break_in_loop(void) {
    value_t result;

    // Continue and break in infinite loop
    result = run_continue_test("var n = 0\n"
                               "var count = 0\n"
                               "loop\n"
                               "    n = n + 1\n"
                               "    if n > 20 then break\n"
                               "    if n mod 4 == 0 then continue\n"
                               "    count = count + 1\n"
                               "end loop\n"
                               "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // 20 iterations, skip 5 (4,8,12,16,20)
    vm_release(result);
}

// Test continue in single-line if expression
void test_continue_in_single_line_if(void) {
    value_t result;

    // Continue in single-line if with then
    result = run_continue_test("var sum = 0\n"
                               "var i = 0\n"
                               "while i < 10 do\n"
                               "    i = i + 1\n"
                               "    if i == 3 or i == 7 then continue\n"
                               "    sum = sum + i\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(45, result.as.int32); // 1+2+4+5+6+8+9+10 = 45 (skip 3,7)
    vm_release(result);
}

// Test continue skipping all even numbers in a range
void test_continue_skip_evens(void) {
    value_t result;

    // Sum only odd numbers
    result = run_continue_test("var sum = 0\n"
                               "var n = 0\n"
                               "while n < 10 do\n"
                               "    n = n + 1\n"
                               "    if n mod 2 == 0 then continue\n"
                               "    sum = sum + n\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32); // 1+3+5+7+9 = 25
    vm_release(result);
}

// Test continue with nested if conditions
void test_continue_with_nested_if(void) {
    value_t result;

    result = run_continue_test("var count = 0\n"
                               "var sum = 0\n"
                               "while count < 15 do\n"
                               "    count = count + 1\n"
                               "    if count > 5 then\n"
                               "        if count < 10 then continue\n"
                               "    sum = sum + count\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(90, result.as.int32); // 1+2+3+4+5+10+11+12+13+14+15 = 90
    vm_release(result);
}

// Test continue as expression in assignment (like break)
void test_continue_as_expression(void) {
    value_t result;

    // Test that continue works in expression context
    result = run_continue_test("var i = 0\n"
                               "var skipped = 0\n"
                               "while i < 10 do\n"
                               "    i = i + 1\n"
                               "    skipped = if i mod 3 == 0 then continue else skipped\n"
                               "    skipped = skipped + 1\n"
                               "end while\n"
                               "i - skipped");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32); // 10 iterations, 3 skipped (3,6,9)
    vm_release(result);
}

// Test continue behavior at loop boundaries
void test_continue_at_boundaries(void) {
    value_t result;

    // Continue on first iteration
    result = run_continue_test("var sum = 0\n"
                               "var i = 0\n"
                               "while i < 5 do\n"
                               "    i = i + 1\n"
                               "    if i == 1 then continue\n"
                               "    sum = sum + i\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32); // 2+3+4+5 = 14

    // Continue on last iteration
    result = run_continue_test("var sum = 0\n"
                               "var i = 0\n"
                               "while i < 5 do\n"
                               "    i = i + 1\n"
                               "    if i == 5 then continue\n"
                               "    sum = sum + i\n"
                               "end while\n"
                               "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32); // 1+2+3+4 = 10
    vm_release(result);
}

// Test suite runner
void test_continue_statements_suite(void) {
    RUN_TEST(test_continue_in_while_loop_skip_odd);
    RUN_TEST(test_continue_in_infinite_loop);
    RUN_TEST(test_continue_with_complex_conditions);
    RUN_TEST(test_continue_and_break_in_while);
    RUN_TEST(test_continue_and_break_in_loop);
    RUN_TEST(test_continue_in_single_line_if);
    RUN_TEST(test_continue_skip_evens);
    RUN_TEST(test_continue_with_nested_if);
    RUN_TEST(test_continue_as_expression);
    RUN_TEST(test_continue_at_boundaries);
}
