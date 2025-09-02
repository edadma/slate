#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>


// ===========================
// RANGE CLASS BASIC TESTS
// ===========================

// Test range construction and basic properties
void test_range_construction(void) {
    // Test inclusive range
    value_t result = test_execute_expression("1..5");
    TEST_ASSERT_EQUAL(VAL_RANGE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.range);
    vm_release(result);
    
    // Test exclusive range
    result = test_execute_expression("1..<5");
    TEST_ASSERT_EQUAL(VAL_RANGE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.range);
    vm_release(result);
}

// Test range type checking
void test_range_type_checking(void) {
    value_t result = test_execute_expression("type(1..5)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("range", result.as.string);
    vm_release(result);
}

// ===========================
// RANGE METHOD TESTS
// ===========================

void test_range_start_end_value(void) {
    value_t result = test_execute_expression("(1..5).start()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("(1..5).endValue()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("(10..20).start()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(10, result.as.int32);
    vm_release(result);
}

void test_range_is_exclusive(void) {
    value_t result = test_execute_expression("(1..5).isExclusive()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // inclusive
    vm_release(result);
    
    result = test_execute_expression("(1..<5).isExclusive()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // exclusive
    vm_release(result);
}

void test_range_is_empty(void) {
    // Normal ranges are not empty
    value_t result = test_execute_expression("(1..5).isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
    
    // Single-element inclusive range is not empty
    result = test_execute_expression("(5..5).isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
    
    // Single-element exclusive range is empty
    result = test_execute_expression("(5..<5).isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    // Backwards range is empty
    result = test_execute_expression("(5..1).isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
}

void test_range_length(void) {
    value_t result = test_execute_expression("(1..5).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32); // 1,2,3,4,5
    vm_release(result);
    
    result = test_execute_expression("(1..<5).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(4, result.as.int32); // 1,2,3,4
    vm_release(result);
    
    result = test_execute_expression("(10..10).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32); // just 10
    vm_release(result);
    
    result = test_execute_expression("(10..<10).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32); // empty
    vm_release(result);
}

void test_range_contains(void) {
    value_t result = test_execute_expression("(1..5).contains(3)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true
    vm_release(result);
    
    result = test_execute_expression("(1..5).contains(5)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true (inclusive)
    vm_release(result);
    
    result = test_execute_expression("(1..<5).contains(5)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false (exclusive)
    vm_release(result);
    
    result = test_execute_expression("(1..5).contains(0)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false
    vm_release(result);
    
    result = test_execute_expression("(1..5).contains(6)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false
    vm_release(result);
}

void test_range_to_array(void) {
    value_t result = test_execute_expression("(1..3).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements [1, 2, 3]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    vm_release(result);
    
    result = test_execute_expression("(1..<3).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(2, da_length(result.as.array)); // [1, 2]
    
    elem0 = (value_t*)da_get(result.as.array, 0);
    elem1 = (value_t*)da_get(result.as.array, 1);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    vm_release(result);
    
    // Empty range
    result = test_execute_expression("(5..<5).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
}

void test_range_reverse(void) {
    value_t result = test_execute_expression("(1..5).reverse().start()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32); // start of reversed range
    vm_release(result);
    
    result = test_execute_expression("(1..5).reverse().endValue()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32); // end of reversed range
    vm_release(result);
    
    result = test_execute_expression("(1..<5).reverse().isExclusive()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // maintains exclusivity
    vm_release(result);
}

void test_range_equals(void) {
    value_t result = test_execute_expression("(1..5).equals(1..5)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // equal ranges
    vm_release(result);
    
    result = test_execute_expression("(1..5).equals(1..<5)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // different exclusivity
    vm_release(result);
    
    result = test_execute_expression("(1..5).equals(2..5)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // different start
    vm_release(result);
    
    result = test_execute_expression("(1..5).equals(1..6)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // different end
    vm_release(result);
}

void test_range_iterator(void) {
    value_t result = test_execute_expression("(1..3).iterator()");
    TEST_ASSERT_EQUAL(VAL_ITERATOR, result.type);
    vm_release(result);
    
    // Test iterator functionality through hasNext/next
    // This is already tested in existing iterator tests, so just verify creation
}

// ===========================
// RANGE ITERATOR COMPREHENSIVE TESTS
// ===========================

void test_range_iterator_forward_inclusive(void) {
    // Test forward inclusive range iterator (1..3)
    value_t result = test_execute_expression("var it = (1..3).iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true
    vm_release(result);
    
    // Get first element (1)
    result = test_execute_expression("var it = (1..3).iterator(); it.next()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32);
    vm_release(result);
    
    // Test full iteration sequence
    result = test_execute_expression("var it = (1..3).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements [1, 2, 3]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    vm_release(result);
}

void test_range_iterator_forward_exclusive(void) {
    // Test forward exclusive range iterator (1..<4)
    value_t result = test_execute_expression("var it = (1..<4).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    // Check elements [1, 2, 3] (4 excluded)
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    vm_release(result);
}

void test_range_iterator_reverse_inclusive(void) {
    // Test reverse inclusive range iterator (5..1)
    value_t result = test_execute_expression("var it = (5..1).iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true - should work now!
    vm_release(result);
    
    // Get first element (5)
    result = test_execute_expression("var it = (5..1).iterator(); it.next()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32);
    vm_release(result);
    
    // Test full iteration sequence
    result = test_execute_expression("var it = (5..1).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements [5, 4, 3, 2, 1]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    value_t* elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(5, elem0->as.int32);
    TEST_ASSERT_EQUAL(4, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    TEST_ASSERT_EQUAL(2, elem3->as.int32);
    TEST_ASSERT_EQUAL(1, elem4->as.int32);
    vm_release(result);
}

void test_range_iterator_reverse_exclusive(void) {
    // Test reverse exclusive range iterator (5..<1)
    value_t result = test_execute_expression("var it = (5..<1).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(4, da_length(result.as.array));
    
    // Check elements [5, 4, 3, 2] (1 excluded)
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    TEST_ASSERT_EQUAL(5, elem0->as.int32);
    TEST_ASSERT_EQUAL(4, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    TEST_ASSERT_EQUAL(2, elem3->as.int32);
    vm_release(result);
}

void test_range_iterator_edge_cases(void) {
    // Single element forward range (5..5)
    value_t result = test_execute_expression("var it = (5..5).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(5, elem0->as.int32);
    vm_release(result);
    
    // Empty exclusive range (5..<5)
    result = test_execute_expression("var it = (5..<5).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
    
    // Test isEmpty() method on iterators
    result = test_execute_expression("var it = (5..<5).iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true - empty
    vm_release(result);
    
    result = test_execute_expression("var it = (5..5).iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false - has one element
    vm_release(result);
}

void test_range_iterator_negative_numbers(void) {
    // Negative to positive range (-2..2)
    value_t result = test_execute_expression("var it = (-2..2).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements [-2, -1, 0, 1, 2]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    value_t* elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(-2, elem0->as.int32);
    TEST_ASSERT_EQUAL(-1, elem1->as.int32);
    TEST_ASSERT_EQUAL(0, elem2->as.int32);
    TEST_ASSERT_EQUAL(1, elem3->as.int32);
    TEST_ASSERT_EQUAL(2, elem4->as.int32);
    vm_release(result);
    
    // Reverse negative range (2..-2)
    result = test_execute_expression("var it = (2..-2).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements [2, 1, 0, -1, -2]
    elem0 = (value_t*)da_get(result.as.array, 0);
    elem1 = (value_t*)da_get(result.as.array, 1);
    elem2 = (value_t*)da_get(result.as.array, 2);
    elem3 = (value_t*)da_get(result.as.array, 3);
    elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(2, elem0->as.int32);
    TEST_ASSERT_EQUAL(1, elem1->as.int32);
    TEST_ASSERT_EQUAL(0, elem2->as.int32);
    TEST_ASSERT_EQUAL(-1, elem3->as.int32);
    TEST_ASSERT_EQUAL(-2, elem4->as.int32);
    vm_release(result);
}

// ===========================
// RANGE COMPREHENSIVE TESTS
// ===========================

// Test range method chaining
void test_range_method_chaining(void) {
    // Test the original failing case: (1..5).reverse().toArray() should now work!
    value_t result = test_execute_expression("(1..5).reverse().toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements are reversed [5, 4, 3, 2, 1]
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    value_t* elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(5, elem0->as.int32);
    TEST_ASSERT_EQUAL(4, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    TEST_ASSERT_EQUAL(2, elem3->as.int32);
    TEST_ASSERT_EQUAL(1, elem4->as.int32);
    vm_release(result);
    
    // Test that Array(range) now treats range as single element (no expansion)
    result = test_execute_expression("Array((1..5).reverse())");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array)); // Single range element
    
    // The array should contain the range as a single element
    value_t* range_elem = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(VAL_RANGE, range_elem->type);
    vm_release(result);
}

// Test range edge cases
void test_range_edge_cases(void) {
    // Zero-based range
    value_t result = test_execute_expression("(0..2).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(3, result.as.int32);
    vm_release(result);
    
    // Negative ranges
    result = test_execute_expression("(-2..2).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32); // -2,-1,0,1,2
    vm_release(result);
    
    // Large range
    result = test_execute_expression("(100..102).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(3, result.as.int32);
    vm_release(result);
}

// Test Suite Runner
void test_class_range_suite(void) {
    RUN_TEST(test_range_construction);
    RUN_TEST(test_range_type_checking);
    RUN_TEST(test_range_start_end_value);
    RUN_TEST(test_range_is_exclusive);
    RUN_TEST(test_range_is_empty);
    RUN_TEST(test_range_length);
    RUN_TEST(test_range_contains);
    RUN_TEST(test_range_to_array);
    RUN_TEST(test_range_reverse);
    RUN_TEST(test_range_equals);
    RUN_TEST(test_range_iterator);
    RUN_TEST(test_range_method_chaining);
    RUN_TEST(test_range_edge_cases);
    
    // Range iterator comprehensive tests
    RUN_TEST(test_range_iterator_forward_inclusive);
    RUN_TEST(test_range_iterator_forward_exclusive);
    RUN_TEST(test_range_iterator_reverse_inclusive);
    RUN_TEST(test_range_iterator_reverse_exclusive);
    RUN_TEST(test_range_iterator_edge_cases);
    RUN_TEST(test_range_iterator_negative_numbers);
}