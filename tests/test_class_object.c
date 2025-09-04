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

// Test Object.hash() method
void test_object_hash_basic(void) {
    // Test that objects have hash method and return int32
    value_t result = test_execute_expression("{a: 1, b: 2}.hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    vm_release(result);
}

void test_object_hash_empty(void) {
    // Test empty object hash
    value_t result = test_execute_expression("{}.hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    vm_release(result);
}

void test_object_hash_consistency(void) {
    // Test that same objects produce same hash
    value_t result1 = test_execute_expression("{a: 1, b: 2}.hash()");
    value_t result2 = test_execute_expression("{a: 1, b: 2}.hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_EQUAL_INT32(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_object_hash_order_independent(void) {
    // Test that key order doesn't affect hash
    value_t result1 = test_execute_expression("{a: 1, b: 2}.hash()");
    value_t result2 = test_execute_expression("{b: 2, a: 1}.hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_EQUAL_INT32(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_object_hash_value_sensitive(void) {
    // Test that different values produce different hashes
    value_t result1 = test_execute_expression("{a: 1}.hash()");
    value_t result2 = test_execute_expression("{a: 2}.hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_NOT_EQUAL(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_object_method_hash_equality(void) {
    // Test the exact pattern requested: {x: 5}.hash() == {x: 5}.hash()
    value_t result = test_execute_expression("{x: 5}.hash() == {x: 5}.hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
    
    // Test more complex objects
    result = test_execute_expression("{a: 1, b: \"hello\", c: true}.hash() == {a: 1, b: \"hello\", c: true}.hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
    
    // Test that different objects have different hashes
    result = test_execute_expression("{x: 5}.hash() == {x: 6}.hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
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
    RUN_TEST(test_object_hash_basic);
    RUN_TEST(test_object_hash_empty);
    RUN_TEST(test_object_hash_consistency);
    RUN_TEST(test_object_hash_order_independent);
    RUN_TEST(test_object_hash_value_sensitive);
    RUN_TEST(test_object_method_hash_equality);
}