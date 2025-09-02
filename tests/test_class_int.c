#include "unity.h"
#include "test_helpers.h"
#include <limits.h>
#include <stdio.h>

// Forward declarations
void test_hexadecimal_literals(void);
void test_hexadecimal_arithmetic(void);


void test_integer_literals_vs_float_literals() {
    // Test basic integer value creation without execute_expression
    value_t result = make_int32(42);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}


void test_int32_overflow_detection() {
    // Test overflow detection helpers from BigInt library
    int32_t result;
    
    // Addition overflow
    TEST_ASSERT_TRUE(di_add_overflow_int32(1000, 2000, &result));
    TEST_ASSERT_EQUAL_INT32(3000, result);
    
    // Addition overflow - should fail
    TEST_ASSERT_FALSE(di_add_overflow_int32(INT32_MAX, 1, &result));
    TEST_ASSERT_FALSE(di_add_overflow_int32(INT32_MIN, -1, &result));
    
    // Multiplication overflow
    TEST_ASSERT_TRUE(di_multiply_overflow_int32(1000, 2000, &result));
    TEST_ASSERT_EQUAL_INT32(2000000, result);
    
    // Multiplication overflow - should fail
    TEST_ASSERT_FALSE(di_multiply_overflow_int32(INT32_MAX, 2, &result));
    TEST_ASSERT_FALSE(di_multiply_overflow_int32(100000, 100000, &result));
    
    // Subtraction overflow  
    TEST_ASSERT_TRUE(di_subtract_overflow_int32(1000, 500, &result));
    TEST_ASSERT_EQUAL_INT32(500, result);
    
    // Subtraction overflow - should fail
    TEST_ASSERT_FALSE(di_subtract_overflow_int32(INT32_MIN, 1, &result));
}

void test_bigint_creation() {
    // Test creating BigInt from int32
    di_int big = di_from_int32(42);
    TEST_ASSERT_NOT_NULL(big);
    
    int32_t result;
    TEST_ASSERT_TRUE(di_to_int32(big, &result));
    TEST_ASSERT_EQUAL_INT32(42, result);
    
    di_release(&big);
    TEST_ASSERT_NULL(big);
    
    // Test creating BigInt from large number
    big = di_from_int64(5000000000LL);  // Larger than int32
    TEST_ASSERT_NOT_NULL(big);
    
    int32_t small_result;
    TEST_ASSERT_FALSE(di_to_int32(big, &small_result));  // Should fail
    
    int64_t large_result;
    TEST_ASSERT_TRUE(di_to_int64(big, &large_result));
    TEST_ASSERT_EQUAL_INT64(5000000000LL, large_result);
    
    di_release(&big);
}

void test_bigint_arithmetic() {
    // Test BigInt addition
    di_int a = di_from_int32(1000);
    di_int b = di_from_int32(2000);
    di_int sum = di_add(a, b);
    
    TEST_ASSERT_NOT_NULL(sum);
    
    int32_t result;
    TEST_ASSERT_TRUE(di_to_int32(sum, &result));
    TEST_ASSERT_EQUAL_INT32(3000, result);
    
    di_release(&a);
    di_release(&b);
    di_release(&sum);
}

void test_bigint_reference_counting() {
    // Test reference counting works properly
    di_int big = di_from_int32(42);
    TEST_ASSERT_EQUAL_size_t(1, di_ref_count(big));
    
    di_int retained = di_retain(big);
    TEST_ASSERT_EQUAL_size_t(2, di_ref_count(big));
    TEST_ASSERT_EQUAL_size_t(2, di_ref_count(retained));
    
    di_release(&retained);
    TEST_ASSERT_NULL(retained);
    TEST_ASSERT_EQUAL_size_t(1, di_ref_count(big));
    
    di_release(&big);
    TEST_ASSERT_NULL(big);
}

void test_vm_integer_value_creation() {
    // Test VM value creation functions
    value_t int_val = make_int32(42);
    TEST_ASSERT_EQUAL(VAL_INT32, int_val.type);
    TEST_ASSERT_EQUAL_INT32(42, int_val.as.int32);
    
    // Test BigInt value creation
    di_int big = di_from_int32(100);
    value_t bigint_val = make_bigint(big);
    TEST_ASSERT_EQUAL(VAL_BIGINT, bigint_val.type);
    TEST_ASSERT_EQUAL_PTR(big, bigint_val.as.bigint);
    
    // Test memory management
    value_t retained = vm_retain(bigint_val);
    TEST_ASSERT_EQUAL(VAL_BIGINT, retained.type);
    TEST_ASSERT_EQUAL_size_t(2, di_ref_count(retained.as.bigint));
    
    vm_release(retained);
    vm_release(bigint_val);
}

void test_integer_truthiness() {
    // Test is_falsy for integers
    value_t zero_int = make_int32(0);
    value_t nonzero_int = make_int32(42);
    
    TEST_ASSERT_TRUE(is_falsy(zero_int));
    TEST_ASSERT_FALSE(is_falsy(nonzero_int));
    
    // Test BigInt truthiness
    di_int zero_big = di_from_int32(0);
    di_int nonzero_big = di_from_int32(100);
    
    value_t zero_bigint = make_bigint(zero_big);
    value_t nonzero_bigint = make_bigint(nonzero_big);
    
    TEST_ASSERT_TRUE(is_falsy(zero_bigint));
    TEST_ASSERT_FALSE(is_falsy(nonzero_bigint));
    
    vm_release(zero_bigint);
    vm_release(nonzero_bigint);
}

void test_integer_equality() {
    // Test int32 equality
    value_t a = make_int32(42);
    value_t b = make_int32(42);
    value_t c = make_int32(100);
    
    TEST_ASSERT_TRUE(values_equal(a, b));
    TEST_ASSERT_FALSE(values_equal(a, c));
    
    // Test cross-type numeric equality (int32 vs number)
    value_t num = make_float64(42.0);
    TEST_ASSERT_TRUE(values_equal(a, num));  // Should be equal
    
    value_t float_num = make_float64(42.5);
    TEST_ASSERT_FALSE(values_equal(a, float_num));  // Should not be equal
}

void test_large_integer_parsing() {
    // Test parsing integers that are too large for int32
    // These should become BigInt
    
    // This will overflow int32 during parsing
    char large_int[32];
    sprintf(large_int, "%lld", (long long)INT32_MAX + 1000LL);
    
    value_t result = test_execute_expression(large_int);
    // Should be parsed as BigInt
    TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
    
    // Verify the actual value is correct
    char* str = di_to_string(result.as.bigint, 10);
    char expected[32];
    sprintf(expected, "%lld", (long long)INT32_MAX + 1000LL);
    TEST_ASSERT_EQUAL_STRING(expected, str);
    free(str);
    vm_release(result);
}


// Test suite function for integration with main test runner
void test_class_int_suite(void) {
    RUN_TEST(test_integer_literals_vs_float_literals);
    RUN_TEST(test_int32_overflow_detection);
    RUN_TEST(test_bigint_creation);
    RUN_TEST(test_bigint_arithmetic);
    RUN_TEST(test_bigint_reference_counting);
    RUN_TEST(test_vm_integer_value_creation);
    RUN_TEST(test_integer_truthiness);
    RUN_TEST(test_integer_equality);
    RUN_TEST(test_large_integer_parsing);
    RUN_TEST(test_hexadecimal_literals);
    RUN_TEST(test_hexadecimal_arithmetic);
}

// Test hexadecimal literal parsing and type handling
void test_hexadecimal_literals(void) {
    // Test basic hex literals parse correctly
    value_t result = test_execute_expression("0x10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(16, result.as.int32);
    
    // Test case insensitive
    result = test_execute_expression("0xFF");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(255, result.as.int32);
    
    result = test_execute_expression("0xff");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(255, result.as.int32);
    
    // Test zero
    result = test_execute_expression("0x0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    
    // Test large hex that should become BigInt
    result = test_execute_expression("0x100000000");
    TEST_ASSERT_EQUAL_INT(VAL_BIGINT, result.type);
    
    // Test hex vs decimal equivalence
    result = test_execute_expression("0xFF == 255");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
}

// Test hexadecimal arithmetic operations
void test_hexadecimal_arithmetic(void) {
    // Test hex arithmetic
    value_t result = test_execute_expression("0xFF + 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(256, result.as.int32);
    
    // Test hex with decimal mixing
    result = test_execute_expression("0x10 * 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(160, result.as.int32);
    
    // Test type function with hex
    result = test_execute_expression("type(0xFF)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("int32", result.as.string);
}