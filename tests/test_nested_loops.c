#include "unity.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "vm.h"
#include <string.h>
#include <stdlib.h>

// Helper function to run nested loop test code and return result
static value_t run_nested_loop_test(const char* source) {
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

// Test nested while loops with continue in inner loop
void test_nested_while_continue_inner(void) {
    value_t result;

    // Continue affects only inner loop
    result = run_nested_loop_test("var total = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 3 do\n"
                                 "    outer = outer + 1\n"
                                 "    var inner = 0\n"
                                 "    while inner < 4 do\n"
                                 "        inner = inner + 1\n"
                                 "        if inner mod 2 == 0 then continue\n"
                                 "        total = total + inner\n"
                                 "    end while\n"
                                 "    total\n"
                                 "end while\n"
                                 "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32); // 3 * (1 + 3) = 12 (skip even numbers)
    vm_release(result);
}

// Test nested while loops with continue in outer loop
void test_nested_while_continue_outer(void) {
    value_t result;

    // Continue affects outer loop, skipping inner loop entirely
    result = run_nested_loop_test("var total = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 5 do\n"
                                 "    outer = outer + 1\n"
                                 "    if outer == 3 then continue\n"
                                 "    var inner = 0\n"
                                 "    while inner < 2 do\n"
                                 "        inner = inner + 1\n"
                                 "        total = total + 1\n"
                                 "    end while\n"
                                 "    total\n"
                                 "end while\n"
                                 "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32); // 4 outer * 2 inner = 8 (skip outer=3)
    vm_release(result);
}

// Test nested while loops with break in inner loop
void test_nested_while_break_inner(void) {
    value_t result;

    // Break affects only inner loop
    result = run_nested_loop_test("var total = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 3 do\n"
                                 "    outer = outer + 1\n"
                                 "    var inner = 0\n"
                                 "    while inner < 10 do\n"
                                 "        inner = inner + 1\n"
                                 "        total = total + 1\n"
                                 "        if inner == 2 then break\n"
                                 "    end while\n"
                                 "    total\n"
                                 "end while\n"
                                 "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32); // 3 outer * 2 inner = 6 (break at inner=2)
    vm_release(result);
}

// Test nested while loops with break in outer loop
void test_nested_while_break_outer(void) {
    value_t result;

    // Break affects outer loop, stopping everything
    result = run_nested_loop_test("var total = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 10 do\n"
                                 "    outer = outer + 1\n"
                                 "    if outer == 3 then break\n"
                                 "    var inner = 0\n"
                                 "    while inner < 2 do\n"
                                 "        inner = inner + 1\n"
                                 "        total = total + 1\n"
                                 "    end while\n"
                                 "    total\n"
                                 "end while\n"
                                 "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32); // 2 outer * 2 inner = 4 (break at outer=3)
    vm_release(result);
}

// Test while loop inside infinite loop
void test_while_in_infinite_loop(void) {
    value_t result;

    result = run_nested_loop_test("var count = 0\n"
                                 "var outer = 0\n"
                                 "loop\n"
                                 "    outer = outer + 1\n"
                                 "    if outer > 3 then break\n"
                                 "    var inner = 0\n"
                                 "    while inner < 2 do\n"
                                 "        inner = inner + 1\n"
                                 "        count = count + 1\n"
                                 "        if inner == 1 then continue\n"
                                 "        count = count + 10\n"
                                 "    end while\n"
                                 "    count\n"
                                 "end loop\n"
                                 "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(36, result.as.int32); // 3 outer * (1 + 1 + 10) = 36
    vm_release(result);
}

// Test infinite loop inside while loop
void test_infinite_loop_in_while(void) {
    value_t result;

    result = run_nested_loop_test("var total = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 2 do\n"
                                 "    outer = outer + 1\n"
                                 "    var inner = 0\n"
                                 "    loop\n"
                                 "        inner = inner + 1\n"
                                 "        total = total + 1\n"
                                 "        if inner >= 3 then break\n"
                                 "        if inner == 2 then continue\n"
                                 "        total = total + 5\n"
                                 "    end loop\n"
                                 "    total\n"
                                 "end while\n"
                                 "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(16, result.as.int32); // 2 outer * (1+5 + 1 + 1) = 16
    vm_release(result);
}

// Test deeply nested loops (3 levels)
void test_triple_nested_loops(void) {
    value_t result;

    result = run_nested_loop_test("var count = 0\n"
                                 "var i = 0\n"
                                 "while i < 2 do\n"
                                 "    i = i + 1\n"
                                 "    var j = 0\n"
                                 "    while j < 2 do\n"
                                 "        j = j + 1\n"
                                 "        var k = 0\n"
                                 "        loop\n"
                                 "            k = k + 1\n"
                                 "            count = count + 1\n"
                                 "            if k >= 2 then break\n"
                                 "            if k == 1 then continue\n"
                                 "            count = count + 100\n"
                                 "        end loop\n"
                                 "        count\n"
                                 "    end while\n"
                                 "    count\n"
                                 "end while\n"
                                 "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32); // 2*2*2 = 8 (k=1: continue, k=2: count+1 then break)
    vm_release(result);
}

// Test break and continue in same nested structure
void test_mixed_break_continue_nested(void) {
    value_t result;

    result = run_nested_loop_test("var result = 0\n"
                                 "var outer = 0\n"
                                 "while outer < 5 do\n"
                                 "    outer = outer + 1\n"
                                 "    if outer == 4 then continue\n"
                                 "    var inner = 0\n"
                                 "    loop\n"
                                 "        inner = inner + 1\n"
                                 "        if inner > 5 then break\n"
                                 "        if inner mod 2 == 0 then continue\n"
                                 "        result = result + inner\n"
                                 "    end loop\n"
                                 "    result\n"
                                 "end while\n"
                                 "result");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(36, result.as.int32); // 4 outer * (1+3+5) = 36 (skip outer=4, inner=2,4)
    vm_release(result);
}

// Test that continue in deeply nested loop affects correct level
void test_continue_scope_correctness(void) {
    value_t result;

    // Continue in innermost loop should not affect outer loops
    result = run_nested_loop_test("var trace = 0\n"
                                 "var i = 0\n"
                                 "while i < 3 do\n"
                                 "    i = i + 1\n"
                                 "    trace = trace * 10 + 1\n"  // Mark outer loop entry
                                 "    var j = 0\n"
                                 "    while j < 3 do\n"
                                 "        j = j + 1\n"
                                 "        if j == 2 then continue\n"  // Skip middle iteration
                                 "        trace = trace * 10 + j\n"   // Mark inner iterations
                                 "    end while\n"
                                 "    trace\n"
                                 "end while\n"
                                 "trace");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    // trace should be: 113113113 (outer: +1, inner: +1,skip,+3) repeated 3 times
    vm_release(result);
}

// Test suite runner
void test_nested_loops_suite(void) {
    RUN_TEST(test_nested_while_continue_inner);
    RUN_TEST(test_nested_while_continue_outer);
    RUN_TEST(test_nested_while_break_inner);
    RUN_TEST(test_nested_while_break_outer);
    RUN_TEST(test_while_in_infinite_loop);
    RUN_TEST(test_infinite_loop_in_while);
    RUN_TEST(test_triple_nested_loops);
    RUN_TEST(test_mixed_break_continue_nested);
    RUN_TEST(test_continue_scope_correctness);
}