#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>

// Helper function to run code and get result
static value_t run_code(const char* code) {
    return test_execute_expression(code);
}

// ===========================
// ARRAY CONSTRUCTOR TESTS
// ===========================

// Test Array() - empty array constructor
void test_array_constructor_empty(void) {
    value_t result = run_code("Array()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
}

// Test Array(1, 2, 3) - multiple arguments constructor
void test_array_constructor_multiple_args(void) {
    value_t result = run_code("Array(1, 2, 3)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    
    TEST_ASSERT_EQUAL(VAL_INT32, elem0->type);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(VAL_INT32, elem1->type);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(VAL_INT32, elem2->type);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    
    vm_release(result);
}

// Test Array([1, 2, 3]) - array copy constructor
void test_array_constructor_copy_array(void) {
    value_t result = run_code("Array([1, 2, 3])");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    
    TEST_ASSERT_EQUAL(VAL_INT32, elem0->type);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(VAL_INT32, elem1->type);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(VAL_INT32, elem2->type);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    
    vm_release(result);
}

// Test Array([]) - empty array copy
void test_array_constructor_copy_empty_array(void) {
    value_t result = run_code("Array([])");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
}


// Test Array("hello") - single non-array argument
void test_array_constructor_single_element(void) {
    value_t result = run_code("Array(\"hello\")");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array));
    
    value_t* elem = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(VAL_STRING, elem->type);
    TEST_ASSERT_EQUAL_STRING("hello", elem->as.string);
    
    vm_release(result);
}

// Test Array(42) - single number argument  
void test_array_constructor_single_number(void) {
    value_t result = run_code("Array(42)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array));
    
    value_t* elem = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
    TEST_ASSERT_EQUAL(42, elem->as.int32);
    
    vm_release(result);
}

// Test Array(true, false, null) - mixed types
void test_array_constructor_mixed_types(void) {
    value_t result = run_code("Array(true, false, null)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, elem0->type);
    TEST_ASSERT_TRUE(elem0->as.boolean);
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, elem1->type);
    TEST_ASSERT_FALSE(elem1->as.boolean);
    TEST_ASSERT_EQUAL(VAL_NULL, elem2->type);
    
    vm_release(result);
}

// ===========================
// ARRAY COPY INDEPENDENCE TESTS
// ===========================

// Test that Array([1,2,3]) creates an independent copy
void test_array_constructor_independence(void) {
    value_t result = run_code("var original = [1, 2]; var copy = Array(original); original.push(3); copy");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(2, da_length(result.as.array)); // Copy should still have 2 elements
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    
    TEST_ASSERT_EQUAL(VAL_INT32, elem0->type);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(VAL_INT32, elem1->type);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    
    vm_release(result);
}

// ===========================
// ARRAY METHOD COMPATIBILITY TESTS
// ===========================

// Test that Array() result has proper methods
void test_array_constructor_methods(void) {
    value_t result = run_code("Array(1, 2, 3).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(3, result.as.int32);
    vm_release(result);
}

// Test Array constructor with push method
void test_array_constructor_with_push(void) {
    value_t result = run_code("var arr = Array(); arr.push(42); arr.length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32);
    vm_release(result);
}

// Test Array constructor with method chaining
void test_array_constructor_method_chaining(void) {
    // Note: push() returns length, not the array, so we test a different chain
    value_t result = run_code("Array(1, 2, 3).copy().length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(3, result.as.int32);
    vm_release(result);
}

// ===========================
// ARRAY FILL METHOD TESTS
// ===========================

// Test Array fill static method exists
void test_array_fill_method_exists(void) {
    // Test that fill static method exists on Array class and returns a native function
    value_t result = run_code("Array.fill");
    TEST_ASSERT_EQUAL(VAL_NATIVE, result.type);
    vm_release(result);
}

// Test Array fill static method with zero size and null function
void test_array_fill_zero_size(void) {
    // Test zero size - should create empty array regardless of function validity
    value_t result = run_code("Array.fill(0, null)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
}

// Note: Full functional tests for Array.fill with working functions will be 
// enabled once function compilation is fully implemented. The method exists,
// validates arguments correctly, and will work once functions are complete.

// =====================================
// ARRAY TESTS MOVED FROM test_builtins.c
// =====================================

// Helper function to interpret a single expression and return result (from test_builtins.c)
static value_t interpret_expression(const char* source) {
    return test_execute_expression(source);
}

void test_array_with_strings(void) {
    value_t result = interpret_expression("[1, \"hello\", true, null]");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    // Convert to string to check display format
    value_t str_result = interpret_expression("\"\" + [1, \"hello\", true, null]");
    TEST_ASSERT_EQUAL(VAL_STRING, str_result.type);
    TEST_ASSERT_EQUAL_STRING("[1, \"hello\", true, null]", str_result.as.string);
    vm_release(result);
    vm_release(str_result);
}

// Test array copy method
void test_array_copy(void) {
    value_t result = interpret_expression("[1, 2, 3].copy()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(VAL_INT32, elem0->type);
    TEST_ASSERT_EQUAL(VAL_INT32, elem1->type);
    TEST_ASSERT_EQUAL(VAL_INT32, elem2->type);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    vm_release(result);
}

// Test array slice method with start and end
void test_array_slice_start_end(void) {
    value_t result = interpret_expression("[1, 2, 3, 4, 5].slice(1, 4)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements [2, 3, 4]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(2, elem0->as.int32);
    TEST_ASSERT_EQUAL(3, elem1->as.int32);
    TEST_ASSERT_EQUAL(4, elem2->as.int32);
    vm_release(result);
}

// Test array slice method with only start
void test_array_slice_start_only(void) {
    value_t result = interpret_expression("[1, 2, 3, 4, 5].slice(2)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements [3, 4, 5]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(3, elem0->as.int32);
    TEST_ASSERT_EQUAL(4, elem1->as.int32);
    TEST_ASSERT_EQUAL(5, elem2->as.int32);
    vm_release(result);
}

// Test array slice method with negative indices
void test_array_slice_negative_indices(void) {
    value_t result = interpret_expression("[1, 2, 3].slice(-2)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(2, da_length(result.as.array));
    
    // Check elements [2, 3]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    TEST_ASSERT_EQUAL(2, elem0->as.int32);
    TEST_ASSERT_EQUAL(3, elem1->as.int32);
    vm_release(result);
}

// Test array slice edge cases
void test_array_slice_edge_cases(void) {
    // Empty slice
    value_t result = interpret_expression("[1, 2, 3].slice(1, 1)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
    
    // Out of bounds indices
    result = interpret_expression("[1, 2, 3].slice(10, 20)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
    
    // Negative start, positive end
    result = interpret_expression("[1, 2, 3, 4].slice(-3, 3)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(2, da_length(result.as.array));
    vm_release(result);
}

// Test array reverse method
void test_array_reverse(void) {
    // Test requires variable support for proper testing
    // For now, we'll test a copy+reverse to avoid modifying original in tests
    value_t result = interpret_expression("[1, 2, 3].copy().reverse()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements are reversed [3, 2, 1]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(3, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(1, elem2->as.int32);
    vm_release(result);
}

// Test Suite Runner
// Test Array.hash() method
void test_array_hash_basic(void) {
    // Test basic array hash
    value_t result = test_execute_expression("[1, 2, 3].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_NOT_EQUAL(0, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("[\"a\", \"b\"].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_NOT_EQUAL(0, result.as.int32);
    vm_release(result);
}

void test_array_hash_empty(void) {
    // Test empty array hash
    value_t result = test_execute_expression("[].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    vm_release(result);
}

void test_array_hash_consistency(void) {
    // Test that same arrays produce same hash
    value_t result1 = test_execute_expression("[1, 2, 3].hash()");
    value_t result2 = test_execute_expression("[1, 2, 3].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_EQUAL_INT32(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_array_hash_order_matters(void) {
    // Test that order matters in array hashing
    value_t result1 = test_execute_expression("[1, 2, 3].hash()");
    value_t result2 = test_execute_expression("[3, 2, 1].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_NOT_EQUAL(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
    
    // Test different lengths
    result1 = test_execute_expression("[1, 2].hash()");
    result2 = test_execute_expression("[1, 2, 3].hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_NOT_EQUAL(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_array_method_hash_equality(void) {
    // Test that identical arrays have equal hash via method call
    value_t result = test_execute_expression("[1, 2, 3].hash() == [1, 2, 3].hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
    
    // Test that different arrays have different hashes
    result = test_execute_expression("[1, 2, 3].hash() == [3, 2, 1].hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    vm_release(result);
    
    // Test empty arrays
    result = test_execute_expression("[].hash() == [].hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
}

// Test Array.equals() method
void test_array_equals_basic(void) {
    value_t result = test_execute_expression("[1, 2, 3].equals([1, 2, 3])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
}

void test_array_equals_different_length(void) {
    value_t result = test_execute_expression("[1, 2].equals([1, 2, 3])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    vm_release(result);
}

void test_array_equals_different_content(void) {
    value_t result = test_execute_expression("[1, 2, 3].equals([1, 2, 4])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    vm_release(result);
}

void test_array_equals_empty(void) {
    value_t result = test_execute_expression("[].equals([])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
}

void test_array_equals_cross_type(void) {
    // Array should not equal non-array types
    value_t result = test_execute_expression("[42].equals(42)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    vm_release(result);
}

void test_array_equals_nested(void) {
    value_t result = test_execute_expression("[[1, 2], [3, 4]].equals([[1, 2], [3, 4]])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
}

void test_array_method_equals_equality(void) {
    // Test the exact pattern requested: [1, 2, 3].equals([1, 2, 3]) == true
    value_t result = test_execute_expression("[1, 2, 3].equals([1, 2, 3]) == true");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
    
    // Test mixed types in arrays
    result = test_execute_expression("[1, \"hello\", true].equals([1, \"hello\", true])");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
}

void test_class_array_suite(void) {
    RUN_TEST(test_array_constructor_empty);
    RUN_TEST(test_array_constructor_multiple_args);
    RUN_TEST(test_array_constructor_copy_array);
    RUN_TEST(test_array_constructor_copy_empty_array);
    RUN_TEST(test_array_constructor_single_element);
    RUN_TEST(test_array_constructor_single_number);
    RUN_TEST(test_array_constructor_mixed_types);
    RUN_TEST(test_array_constructor_independence);
    RUN_TEST(test_array_constructor_methods);
    RUN_TEST(test_array_constructor_with_push);
    RUN_TEST(test_array_constructor_method_chaining);
    RUN_TEST(test_array_fill_method_exists);
    RUN_TEST(test_array_fill_zero_size);
    
    // Tests moved from test_builtins.c
    RUN_TEST(test_array_with_strings);
    RUN_TEST(test_array_copy);
    RUN_TEST(test_array_slice_start_end);
    RUN_TEST(test_array_slice_start_only);
    RUN_TEST(test_array_slice_negative_indices);
    RUN_TEST(test_array_slice_edge_cases);
    RUN_TEST(test_array_reverse);
    
    // Array hash tests
    RUN_TEST(test_array_hash_basic);
    RUN_TEST(test_array_hash_empty);
    RUN_TEST(test_array_hash_consistency);
    RUN_TEST(test_array_hash_order_matters);
    RUN_TEST(test_array_method_hash_equality);
    
    // Array equals tests  
    RUN_TEST(test_array_equals_basic);
    RUN_TEST(test_array_equals_different_length);
    RUN_TEST(test_array_equals_different_content);
    RUN_TEST(test_array_equals_empty);
    RUN_TEST(test_array_equals_cross_type);
    RUN_TEST(test_array_equals_nested);
    RUN_TEST(test_array_method_equals_equality);
}