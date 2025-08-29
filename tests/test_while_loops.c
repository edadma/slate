#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to slate while loop code and return result
static value_t run_while_test(const char* source) {
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

// Test basic while loops (existing functionality)
void test_basic_while_loops(void) {
    value_t result;

    // Simple countdown while loop
    result = run_while_test("var i = 3\n"
                            "while i > 0\n"
                            "    i = i - 1\n"
                            "i");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);

    // While with end marker
    result = run_while_test("var i = 5\n"
                            "while i > 0\n"
                            "    i = i - 1\n"
                            "    i\n"
                            "end while\n"
                            "i");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
}

// Test while loops with modulo and complex conditions
void test_while_loops_with_modulo(void) {
    value_t result;

    // While loop counting multiples of 3 up to 15
    result = run_while_test("var i = 0\n"
                            "var count = 0\n"
                            "while i < 15\n"
                            "    i = i + 1\n"
                            "    if i mod 3 == 0\n"
                            "        count = count + 1\n"
                            "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32); // 3, 6, 9, 12, 15 = 5 multiples
    vm_release(result);

    // While loop with modulo for even numbers
    result = run_while_test("var n = 0\n"
                            "var sum_evens = 0\n"
                            "while n < 10\n"
                            "    if n mod 2 == 0\n"
                            "        sum_evens = sum_evens + n\n"
                            "    n = n + 1\n"
                            "sum_evens");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32); // 0 + 2 + 4 + 6 + 8 = 20
    vm_release(result);

    // While loop with complex modulo condition
    result = run_while_test("var x = 1\n"
                            "while x mod 7 != 0 or x <= 10\n"
                            "    x = x + 1\n"
                            "x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32); // First multiple of 7 > 10
    vm_release(result);
}

// Test while loops with optional 'do' keyword (multi-line)
void test_while_loops_with_do_multiline(void) {
    value_t result;

    // Multi-line while with 'do' keyword
    result = run_while_test("var sum = 0\n"
                            "var i = 1\n"
                            "while i <= 5 do\n"
                            "    sum = sum + i\n"
                            "    i = i + 1\n"
                            "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // 1+2+3+4+5 = 15
    vm_release(result);

    // Multi-line while with 'do' and 'end while'
    result = run_while_test("var product = 1\n"
                            "var i = 1\n"
                            "while i <= 4 do\n"
                            "    product = product * i\n"
                            "    i = i + 1\n"
                            "end while\n"
                            "product");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(24, result.as.int32); // 1*2*3*4 = 24
    vm_release(result);

    // While with 'do' and no 'end while'
    result = run_while_test("var count = 0\n"
                            "while count < 3 do\n"
                            "    count = count + 1\n"
                            "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

// Test single-line while loops with 'do'
void test_single_line_while_loops_with_do(void) {
    value_t result;

    // Simple single-line while with 'do' - counter
    result = run_while_test("var x = 10\n"
                            "while x > 7 do x = x - 1\n"
                            "x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);

    // Single-line while with 'do' - simple increment
    result = run_while_test("var i = 1\n"
                            "while i < 5 do i = i + 1\n"
                            "i");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);

    // Single-line while with modulo condition
    result = run_while_test("var n = 1\n"
                            "while n mod 5 != 0 do n = n + 1\n"
                            "n");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test mixed syntax variations
void test_while_syntax_variations(void) {
    value_t result;

    // Without 'do', multi-line (original syntax)
    result = run_while_test("var a = 2\n"
                            "while a < 5\n"
                            "    a = a + 1\n"
                            "a");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);

    // With 'do', multi-line
    result = run_while_test("var b = 2\n"
                            "while b < 5 do\n"
                            "    b = b + 1\n"
                            "b");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);

    // With 'do', single-line
    result = run_while_test("var c = 2\n"
                            "while c < 5 do c = c + 1\n"
                            "c");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test while loop edge cases
void test_while_loop_edge_cases(void) {
    value_t result;

    // While loop that never executes
    result = run_while_test("var never_run = 42\n"
                            "while false do never_run = 0\n"
                            "never_run");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);

    // While loop with complex boolean expression
    result = run_while_test("var x = 1\n"
                            "var y = 10\n"
                            "while x < 5 and y > 7 do\n"
                            "    x = x + 1\n"
                            "    y = y - 1\n"
                            "x + y");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(11, result.as.int32); // x=4, y=7 -> 4+7=11
    vm_release(result);

    // Nested while loops with 'do'
    result = run_while_test("var total = 0\n"
                            "var i = 1\n"
                            "while i <= 3 do\n"
                            "    var j = 1\n"
                            "    while j <= 2 do\n"
                            "        total = total + 1\n"
                            "        j = j + 1\n"
                            "    i = i + 1\n"
                            "total");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32); // 3 * 2 = 6 iterations
    vm_release(result);
}

// Test suite runner
void test_while_loops_suite(void) {
    RUN_TEST(test_basic_while_loops);
    RUN_TEST(test_while_loops_with_modulo);
    RUN_TEST(test_while_loops_with_do_multiline);
    RUN_TEST(test_single_line_while_loops_with_do);
    RUN_TEST(test_while_syntax_variations);
    RUN_TEST(test_while_loop_edge_cases);
}
