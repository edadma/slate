#include "unity.h"
#include "test_helpers.h"
#include <stdio.h>
#include <string.h>


// Helper to run test and cleanup
static void test_expression_equals_int(const char* source, int expected) {
    value_t result = test_execute_expression(source);
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(expected, result.as.int32);
    vm_release(result);
}

static void test_expression_equals_bool(const char* source, int expected) {
    value_t result = test_execute_expression(source);
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(expected, result.as.boolean);
    vm_release(result);
}

// Ternary operator tests
void test_ternary_true() {
    test_expression_equals_int("true ? 42 : 100", 42);
}

void test_ternary_false() {
    test_expression_equals_int("false ? 42 : 100", 100);
}

void test_ternary_nested() {
    test_expression_equals_int("true ? (false ? 1 : 2) : 3", 2);
}

// Null coalescing operator tests
void test_null_coalesce_null() {
    test_expression_equals_int("null " "?" "?" " 42", 42);
}

void test_null_coalesce_undefined() {
    test_expression_equals_int("undefined " "?" "?" " 42", 42);
}

void test_null_coalesce_value() {
    test_expression_equals_int("100 " "?" "?" " 42", 100);
}

void test_null_coalesce_chain() {
    test_expression_equals_int("null " "?" "?" " undefined " "?" "?" " 42", 42);
}

// Null coalescing assignment tests
void test_null_coalesce_assign_null() {
    test_expression_equals_int("var x = null; x " "?" "?" "= 42; x", 42);
}

void test_null_coalesce_assign_value() {
    test_expression_equals_int("var x = 100; x " "?" "?" "= 42; x", 100);
}

// Shift assignment tests
void test_left_shift_assign() {
    test_expression_equals_int("var x = 5; x <<= 2; x", 20);  // 5 << 2 = 20
}

void test_right_shift_assign() {
    test_expression_equals_int("var x = 20; x >>= 2; x", 5);  // 20 >> 2 = 5
}

void test_logical_right_shift_assign() {
    test_expression_equals_int("var x = -20; x >>>= 28; x", 15);  // -20 >>> 28 = 15 (logical shift)
}

void test_shift_assign_zero() {
    test_expression_equals_int("var x = 0; x <<= 5; x", 0);  // 0 << 5 = 0
}

// Floor division assignment tests
void test_floor_div_assign() {
    test_expression_equals_int("var x = 17; x //= 3; x", 5);  // 17 // 3 = 5
}

void test_floor_div_assign_negative() {
    test_expression_equals_int("var x = -17; x //= 3; x", -6);  // -17 // 3 = -6
}

// Property existence operator tests
void test_in_operator_exists() {
    test_expression_equals_bool("var obj = {a: 1, b: 2}; \"a\" in obj", 1);
}

void test_in_operator_not_exists() {
    test_expression_equals_bool("var obj = {a: 1, b: 2}; \"c\" in obj", 0);
}

void test_in_operator_empty_object() {
    test_expression_equals_bool("var obj = {}; \"x\" in obj", 0);
}

// instanceof operator tests
void test_instanceof_string_class() {
    test_expression_equals_bool("\"hello\" instanceof String", 1);
}

void test_instanceof_localdate_class() {
    test_expression_equals_bool("LocalDate(2024, 12, 25) instanceof LocalDate", 1);
}

void test_instanceof_array_class() {
    test_expression_equals_bool("[1, 2, 3] instanceof Array", 1);
}

void test_instanceof_stringbuilder_class() {
    test_expression_equals_bool("StringBuilder() instanceof StringBuilder", 1);
}

void test_instanceof_range_class() {
    test_expression_equals_bool("(1..10) instanceof Range", 1);
}

void test_instanceof_negative_cases() {
    test_expression_equals_bool("\"hello\" instanceof Array", 0);
    test_expression_equals_bool("[1, 2, 3] instanceof String", 0);
    test_expression_equals_bool("LocalDate(2024, 12, 25) instanceof StringBuilder", 0);
}

void test_instanceof_rejects_primitive_type_names() {
    // This should cause a runtime error
    value_t result = test_execute_expression("42 instanceof \"number\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Runtime error results in null
    vm_release(result);
}

// Complex expression tests
void test_combined_operators() {
    test_expression_equals_int("var x = null; var y = x " "?" "?" " 5; var z = y > 3 ? 100 : 200; z", 100);
}

void test_precedence() {
    test_expression_equals_int("false " "?" "?" " true ? 1 : 2", 2);  // false ?? (true ? 1 : 2)
}

void test_new_operators_suite() {
    // Ternary operator tests
    RUN_TEST(test_ternary_true);
    RUN_TEST(test_ternary_false);
    RUN_TEST(test_ternary_nested);
    
    // Null coalescing tests
    RUN_TEST(test_null_coalesce_null);
    RUN_TEST(test_null_coalesce_undefined);
    RUN_TEST(test_null_coalesce_value);
    RUN_TEST(test_null_coalesce_chain);
    
    // Null coalescing assignment tests
    RUN_TEST(test_null_coalesce_assign_null);
    RUN_TEST(test_null_coalesce_assign_value);
    
    // Shift assignment tests
    RUN_TEST(test_left_shift_assign);
    RUN_TEST(test_right_shift_assign);
    RUN_TEST(test_logical_right_shift_assign);
    RUN_TEST(test_shift_assign_zero);
    
    // Floor division assignment tests
    RUN_TEST(test_floor_div_assign);
    RUN_TEST(test_floor_div_assign_negative);
    
    // Property existence tests
    RUN_TEST(test_in_operator_exists);
    RUN_TEST(test_in_operator_not_exists);
    RUN_TEST(test_in_operator_empty_object);
    
    // instanceof operator tests
    RUN_TEST(test_instanceof_string_class);
    RUN_TEST(test_instanceof_localdate_class);
    RUN_TEST(test_instanceof_array_class);
    RUN_TEST(test_instanceof_stringbuilder_class);
    RUN_TEST(test_instanceof_range_class);
    RUN_TEST(test_instanceof_negative_cases);
    RUN_TEST(test_instanceof_rejects_primitive_type_names);
    
    // Complex expression tests
    RUN_TEST(test_combined_operators);
    RUN_TEST(test_precedence);
}