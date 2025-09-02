#include <stdlib.h>
#include <string.h>
#include "test_helpers.h"
#include "unity.h"


// Test break in single-line if within while loop
void test_break_in_single_line_if(void) {
    value_t result;

    // Test break in single-line if with then
    result = test_execute_expression("var i = 0\n"
                            "while i < 10 do\n"
                            "    i = i + 1\n"
                            "    if i >= 3 then break\n"
                            "end while\n"
                            "i");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

// Test break in multiline if within while loop
void test_break_in_multiline_if(void) {
    value_t result;

    // Test break in multiline if block
    result = test_execute_expression("var sum = 0\n"
                            "var i = 1\n"
                            "while i <= 10 do\n"
                            "    sum = sum + i\n"
                            "    if sum > 10 then\n"
                            "        break\n"
                            "    i = i + 1\n"
                            "end while\n"
                            "sum");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // 1+2+3+4+5 = 15
    vm_release(result);
}

// Test break in nested conditions
void test_break_in_nested_conditions(void) {
    value_t result;

    // Test break in nested if statements
    result = test_execute_expression("var count = 0\n"
                            "while count < 100 do\n"
                            "    count = count + 1\n"
                            "    if count > 5 then\n"
                            "        if count mod 2 == 0 then break\n"
                            "end while\n"
                            "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32); // First even number > 5
    vm_release(result);
}

// Test break with complex conditions
void test_break_with_complex_conditions(void) {
    value_t result;

    // Test break with logical operators in condition
    result = test_execute_expression("var x = 0\n"
                            "var y = 10\n"
                            "while x < 20 do\n"
                            "    x = x + 1\n"
                            "    y = y - 1\n"
                            "    if x >= 7 and y <= 4 then break\n"
                            "end while\n"
                            "x + y");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32); // x=7, y=3 -> 7+3=10
    vm_release(result);
}

// Test break in infinite loop
void test_break_in_infinite_loop(void) {
    value_t result;

    // Test break stopping infinite loop
    result = test_execute_expression("var counter = 0\n"
                            "loop\n"
                            "    counter = counter + 1\n"
                            "    if counter == 5 then break\n"
                            "end loop\n"
                            "counter");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test break with different comparison operators
void test_break_with_various_operators(void) {
    value_t result;

    // Test break with different operators
    result = test_execute_expression("var value = 1\n"
                            "while value <= 20 do\n"
                            "    value = value * 2\n"
                            "    if value > 15 then break\n"
                            "end while\n"
                            "value");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(16, result.as.int32); // 1, 2, 4, 8, 16 (breaks here)
    vm_release(result);

    // Test break with modulo
    result = test_execute_expression("var n = 1\n"
                            "while n < 30 do\n"
                            "    n = n + 1\n"
                            "    if n mod 7 == 0 then break\n"
                            "end while\n"
                            "n");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32); // First multiple of 7
    vm_release(result);
}

// Test break as expression in assignment context
void test_break_as_expression_in_assignment(void) {
    value_t result;

    // Test that break works in expression context within if-then
    result = test_execute_expression("var i = 0\n"
                            "var found = false\n"
                            "while i < 10 do\n"
                            "    i = i + 1\n"
                            "    found = if i == 4 then break else false\n"
                            "end while\n"
                            "i");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

// Test suite runner
void test_break_statements_suite(void) {
    RUN_TEST(test_break_in_single_line_if);
    RUN_TEST(test_break_in_multiline_if);
    RUN_TEST(test_break_in_nested_conditions);
    RUN_TEST(test_break_with_complex_conditions);
    RUN_TEST(test_break_in_infinite_loop);
    RUN_TEST(test_break_with_various_operators);
    RUN_TEST(test_break_as_expression_in_assignment);
}
