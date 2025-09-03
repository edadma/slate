#include <math.h>
#include <string.h>
#include "test_helpers.h"
#include "unity.h"


// Test bitwise AND operator
void test_bitwise_and(void) {
    value_t result;

    // Basic AND operations
    result = test_execute_expression("12 & 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32); // 1100 & 1010 = 1000

    result = test_execute_expression("15 & 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32); // 1111 & 0111 = 0111

    result = test_execute_expression("255 & 240");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(240, result.as.int32); // 11111111 & 11110000 = 11110000

    // AND with zero
    result = test_execute_expression("42 & 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // AND with all bits set
    result = test_execute_expression("42 & -1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    vm_release(result);
}

// Test bitwise OR operator
void test_bitwise_or(void) {
    value_t result;

    // Basic OR operations
    result = test_execute_expression("12 | 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32); // 1100 | 1010 = 1110

    result = test_execute_expression("8 | 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32); // 1000 | 0100 = 1100

    result = test_execute_expression("1 | 2 | 4 | 8");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // Setting individual bits

    // OR with zero
    result = test_execute_expression("42 | 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    vm_release(result);
}

// Test bitwise XOR operator
void test_bitwise_xor(void) {
    value_t result;

    // Basic XOR operations
    result = test_execute_expression("12 ^ 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32); // 1100 ^ 1010 = 0110

    result = test_execute_expression("15 ^ 15");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32); // Same value XOR = 0

    result = test_execute_expression("255 ^ 170");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(85, result.as.int32); // 11111111 ^ 10101010 = 01010101

    // XOR for bit flipping
    result = test_execute_expression("42 ^ -1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(~42, result.as.int32); // XOR with all 1s flips all bits

    vm_release(result);
}

// Test bitwise NOT operator
void test_bitwise_not(void) {
    value_t result;

    // Basic NOT operations
    result = test_execute_expression("~0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-1, result.as.int32); // ~0 = all 1s = -1

    result = test_execute_expression("~-1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32); // ~(-1) = ~(all 1s) = 0

    result = test_execute_expression("~15");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-16, result.as.int32); // ~1111 = ...11110000 = -16

    result = test_execute_expression("~255");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-256, result.as.int32); // ~11111111 = ...00000000 = -256

    vm_release(result);
}

// Test left shift operator
void test_left_shift(void) {
    value_t result;

    // Basic left shifts
    result = test_execute_expression("1 << 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("1 << 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("1 << 8");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(256, result.as.int32);

    result = test_execute_expression("5 << 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32); // 101 << 2 = 10100

    result = test_execute_expression("42 << 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(84, result.as.int32); // Same as multiply by 2

    // Left shift with zero
    result = test_execute_expression("0 << 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    vm_release(result);
}

// Test arithmetic right shift operator (sign-extending)
void test_arithmetic_right_shift(void) {
    value_t result;

    // Basic right shifts with positive numbers
    result = test_execute_expression("8 >> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);

    result = test_execute_expression("20 >> 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);

    result = test_execute_expression("255 >> 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);

    // Arithmetic right shift with negative numbers (sign-extending)
    result = test_execute_expression("-8 >> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-4, result.as.int32); // Sign bit is preserved

    result = test_execute_expression("-1 >> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-1, result.as.int32); // All 1s remain all 1s

    result = test_execute_expression("-16 >> 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-4, result.as.int32); // Sign-extending

    // Right shift by zero
    result = test_execute_expression("42 >> 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    vm_release(result);
}

// Test logical right shift operator (zero-filling)
void test_logical_right_shift(void) {
    value_t result;

    // Basic logical right shifts with positive numbers (same as arithmetic)
    result = test_execute_expression("8 >>> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);

    result = test_execute_expression("20 >>> 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);

    result = test_execute_expression("255 >>> 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);

    // Logical right shift with negative numbers (zero-filling)
    result = test_execute_expression("-8 >>> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2147483644, result.as.int32); // Zero-filled, no sign extension

    result = test_execute_expression("-1 >>> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2147483647, result.as.int32); // 0x7FFFFFFF

    result = test_execute_expression("-1 >>> 31");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32); // Only the sign bit remains

    // Edge case: maximum shift
    result = test_execute_expression("-1 >>> 32");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    // Note: Behavior for shift >= 32 is typically undefined, but let's test what we get
    // This might vary by platform, so we'll just verify it doesn't crash

    vm_release(result);
}

// Test shift operator precedence and combinations
void test_shift_precedence_and_combinations(void) {
    value_t result;

    // Test precedence: shifts have lower precedence than arithmetic
    result = test_execute_expression("2 + 3 << 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32); // (2 + 3) << 1 = 5 << 1 = 10

    result = test_execute_expression("16 >> 1 + 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(
        4, result.as.int32); // 16 >> (1 + 1) = 16 >> 2 = 4 (shifts have lower precedence than addition)

    // Let's test with explicit parentheses to be sure
    result = test_execute_expression("(16 >> 1) + 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(9, result.as.int32); // (16 >> 1) + 1 = 8 + 1 = 9

    // Test combinations of different shift operators
    result = test_execute_expression("(256 >> 2) << 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(128, result.as.int32); // (256 >> 2) << 1 = 64 << 1 = 128

    // Test arithmetic vs logical difference
    result = test_execute_expression("-16 >> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-8, result.as.int32); // Arithmetic (sign-extending)

    result = test_execute_expression("-16 >>> 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2147483640, result.as.int32); // Logical (zero-filling)

    vm_release(result);
}

// Test bitwise operations with complex expressions
void test_bitwise_complex_expressions(void) {
    value_t result;

    // Test chaining bitwise operations
    result = test_execute_expression("15 & 7 | 8");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // (15 & 7) | 8 = 7 | 8 = 15

    result = test_execute_expression("255 ^ 170 ^ 85");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32); // XOR is associative, should cancel out

    // Test precedence: & has higher precedence than |
    result = test_execute_expression("12 | 3 & 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32); // 12 | (3 & 7) = 12 | 3 = 15

    // Test precedence: ^ has precedence between & and |
    result = test_execute_expression("8 | 4 ^ 12");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32); // 8 | (4 ^ 12) = 8 | 8 = 8

    // Test NOT with other operations
    result = test_execute_expression("~0 & 255");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(255, result.as.int32); // (~0) & 255 = (-1) & 255 = 255

    vm_release(result);
}

// Test bitwise operations error cases
void test_bitwise_error_cases(void) {
    value_t result;

    // Test type errors - bitwise operations should only work with integers
    result = test_execute_expression("3.14 & 2");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should be error
    vm_release(result);

    result = test_execute_expression("5 | \"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should be error
    vm_release(result);

    result = test_execute_expression("true ^ false");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should be error
    vm_release(result);

    result = test_execute_expression("~null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should be error
    vm_release(result);

    result = test_execute_expression("42 << 3.5");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should be error
    vm_release(result);
}

// Test short-circuit AND operator (&&)
void test_short_circuit_and_operator(void) {
    value_t result;
    
    // Test JavaScript-like value semantics
    result = test_execute_expression("\"hello\" && \"world\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("world", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"hello\" && null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);
    
    result = test_execute_expression("null && \"backup\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);
    
    result = test_execute_expression("false && \"backup\"");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("true && 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("0 && 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
}

// Test short-circuit OR operator (||)
void test_short_circuit_or_operator(void) {
    value_t result;
    
    // Test JavaScript-like value semantics
    result = test_execute_expression("\"hello\" || \"world\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("null || \"backup\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("backup", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("false || \"backup\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("backup", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("0 || 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("42 || 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
}

// Test short-circuit evaluation behavior (no side effects when short-circuiting)
void test_short_circuit_evaluation_behavior(void) {
    value_t result;
    
    // Note: These tests verify that side effects don't occur when short-circuiting.
    // In real use, we'd capture side effects, but for unit tests we just verify
    // the operators work as expected without causing issues.
    
    // AND short-circuit: false && <anything> should return false without evaluating right side
    result = test_execute_expression("false && undefined");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // OR short-circuit: true || <anything> should return true without evaluating right side  
    result = test_execute_expression("true || undefined");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Complex nested short-circuit
    result = test_execute_expression("(false && undefined) || (true && 42)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
}

// Test &&= logical assignment operator  
void test_logical_and_assignment(void) {
    value_t result;
    
    // Test with truthy left operand - should assign right operand
    result = test_execute_expression("var x = true; x &&= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    // Test with falsy left operand - should keep left operand
    result = test_execute_expression("var x = false; x &&= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("var x = null; x &&= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);
    
    result = test_execute_expression("var x = 0; x &&= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
    
    // Test with string values
    result = test_execute_expression("var x = \"hello\"; x &&= \"world\"; x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("world", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("var x = \"\"; x &&= \"world\"; x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
}

// Test ||= logical assignment operator
void test_logical_or_assignment(void) {
    value_t result;
    
    // Test with falsy left operand - should assign right operand
    result = test_execute_expression("var x = false; x ||= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("var x = null; x ||= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("var x = 0; x ||= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    // Test with truthy left operand - should keep left operand
    result = test_execute_expression("var x = true; x ||= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("var x = 99; x ||= 42; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(99, result.as.int32);
    vm_release(result);
    
    // Test with string values
    result = test_execute_expression("var x = \"\"; x ||= \"default\"; x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("var x = \"hello\"; x ||= \"default\"; x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
}

// Test logical assignment short-circuit behavior (no evaluation of right operand when not needed)
void test_logical_assignment_short_circuit(void) {
    value_t result;
    
    // These tests verify that the right operand is not evaluated when not needed
    // We test this by using values that would cause errors if evaluated
    
    // &&= with falsy left operand - right operand should not be evaluated
    result = test_execute_expression("var x = false; x &&= undefined; x");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // ||= with truthy left operand - right operand should not be evaluated  
    result = test_execute_expression("var x = true; x ||= undefined; x");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test return value of logical assignments
    result = test_execute_expression("var x = false; x &&= 42");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("var x = true; x &&= 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("var x = false; x ||= 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("var x = true; x ||= 42");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
}

// Test complex logical expressions
void test_complex_logical_expressions(void) {
    value_t result;
    
    // Test chaining logical operators
    result = test_execute_expression("true && false || true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("false || false && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test mixed types in logical expressions
    result = test_execute_expression("\"\" || 0 || null || false || \"default\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("1 && 2 && 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
    
    // Test with parentheses
    result = test_execute_expression("(true || false) && (false || true)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("(\"hello\" && \"world\") || (null && \"backup\")");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("world", result.as.string);
    vm_release(result);
}

// Test suite function for integration with main test runner
void test_logical_suite(void) {
    // Bitwise operator tests
    RUN_TEST(test_bitwise_and);
    RUN_TEST(test_bitwise_or);
    RUN_TEST(test_bitwise_xor);
    RUN_TEST(test_bitwise_not);

    // Shift operator tests
    RUN_TEST(test_left_shift);
    RUN_TEST(test_arithmetic_right_shift);
    RUN_TEST(test_logical_right_shift);
    RUN_TEST(test_shift_precedence_and_combinations);

    // Complex expressions and error cases
    RUN_TEST(test_bitwise_complex_expressions);
    RUN_TEST(test_bitwise_error_cases);
    
    // Short-circuit logical operator tests
    RUN_TEST(test_short_circuit_and_operator);
    RUN_TEST(test_short_circuit_or_operator);
    RUN_TEST(test_short_circuit_evaluation_behavior);
    
    // Logical assignment operator tests
    RUN_TEST(test_logical_and_assignment);
    RUN_TEST(test_logical_or_assignment);
    RUN_TEST(test_logical_assignment_short_circuit);
    
    // Complex logical expressions
    RUN_TEST(test_complex_logical_expressions);
}
