#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>


// ===========================
// OBJECT CLASS BASIC TESTS
// ===========================

// Test object construction
void test_object_construction(void) {
    // Test empty object
    value_t result = test_execute_expression("{}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
    
    // Test object with properties
    result = test_execute_expression("{x: 42, y: \"hello\"}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// Test object type checking
void test_object_type_checking(void) {
    value_t result = test_execute_expression("type({})");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("object", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("type({x: 42})");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("object", result.as.string);
    vm_release(result);
}

// Test object property access
void test_object_property_access(void) {
    // Test property access with run_code (simpler than variables)
    value_t result = test_execute_expression("{x: 42, y: \"hello\"}.x");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("{x: 42, y: \"hello\"}.y");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
}

// ===========================
// OBJECT DISPLAY AND CONVERSION TESTS  
// ===========================

// Test object with string values (moved from test_builtins.c)
void test_object_with_string_values(void) {
    value_t result = test_execute_expression("{greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    // Convert to string to check display format
    value_t str_result = test_execute_expression("\"\" + {greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_STRING, str_result.type);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "greeting: \"hello\"") != NULL);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "name: \"world\"") != NULL);
    vm_release(result);
    vm_release(str_result);
}

// Test object string conversion
void test_object_string_conversion(void) {
    // Test empty object string conversion
    value_t result = test_execute_expression("\"\" + {}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "{}") != NULL);
    vm_release(result);
    
    // Test object with number properties
    result = test_execute_expression("\"\" + {x: 1, y: 2}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "x: 1") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "y: 2") != NULL);
    vm_release(result);
}

// ===========================
// OBJECT COMPREHENSIVE TESTS
// ===========================

// Test object with different value types
void test_object_mixed_types(void) {
    value_t result = test_execute_expression("{num: 42, str: \"hello\", bool: true, null_val: null}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// Test nested objects
void test_object_nested(void) {
    value_t result = test_execute_expression("{outer: {inner: 42}}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
    
    // Test nested property access
    result = test_execute_expression("{outer: {inner: 42}}.outer.inner");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test object edge cases
void test_object_edge_cases(void) {
    // Test object with array property
    value_t result = test_execute_expression("{arr: [1, 2, 3]}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
    
    // Test object with range property
    result = test_execute_expression("{range: 1..5}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test Suite Runner
void test_class_object_suite(void) {
    RUN_TEST(test_object_construction);
    RUN_TEST(test_object_type_checking);
    RUN_TEST(test_object_property_access);
    RUN_TEST(test_object_with_string_values);
    RUN_TEST(test_object_string_conversion);
    RUN_TEST(test_object_mixed_types);
    RUN_TEST(test_object_nested);
    RUN_TEST(test_object_edge_cases);
}