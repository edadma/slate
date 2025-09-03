#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>

// ===========================
// STEPPED RANGE TESTS 
// ===========================

// Test basic stepped range construction and parsing
void test_stepped_range_construction(void) {
    // Test basic stepped range creation
    value_t result = test_execute_expression("1..10 step 2");
    TEST_ASSERT_EQUAL(VAL_RANGE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.range);
    
    // Verify the step value is stored correctly
    TEST_ASSERT_EQUAL(VAL_INT32, result.as.range->step.type);
    TEST_ASSERT_EQUAL(2, result.as.range->step.as.int32);
    vm_release(result);
}

// Test stepped range iterator functionality
void test_stepped_range_iterator(void) {
    // Test forward stepped iteration: 1..10 step 2 -> [1, 3, 5, 7, 9]
    value_t result = test_execute_expression("(1..10 step 2).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    value_t* elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(3, elem1->as.int32);
    TEST_ASSERT_EQUAL(5, elem2->as.int32);
    TEST_ASSERT_EQUAL(7, elem3->as.int32);
    TEST_ASSERT_EQUAL(9, elem4->as.int32);
    vm_release(result);
    
    // Test reverse stepped iteration: 10..1 step -2 -> [10, 8, 6, 4, 2]
    result = test_execute_expression("(10..1 step -2).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    elem0 = (value_t*)da_get(result.as.array, 0);
    elem1 = (value_t*)da_get(result.as.array, 1);
    elem2 = (value_t*)da_get(result.as.array, 2);
    elem3 = (value_t*)da_get(result.as.array, 3);
    elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(10, elem0->as.int32);
    TEST_ASSERT_EQUAL(8, elem1->as.int32);
    TEST_ASSERT_EQUAL(6, elem2->as.int32);
    TEST_ASSERT_EQUAL(4, elem3->as.int32);
    TEST_ASSERT_EQUAL(2, elem4->as.int32);
    vm_release(result);
}

// Test manual iterator methods as requested by user
void test_stepped_range_manual_iterator(void) {
    // Test hasNext() and next() with stepped ranges
    value_t result = test_execute_expression("var r = (1..7 step 2); var it = r.iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("var r = (1..7 step 2); var it = r.iterator(); it.next()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32);
    vm_release(result);
    
    // Test complete manual iteration - using indentation-based syntax
    result = test_execute_expression(
        "var r = (1..10 step 3)\n"
        "var it = r.iterator()\n"
        "var values = []\n"
        "while it.hasNext() do\n"
        "    values.push(it.next())\n"
        "values");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(4, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    value_t* elem2 = (value_t*)da_get(result.as.array, 2);
    value_t* elem3 = (value_t*)da_get(result.as.array, 3);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(4, elem1->as.int32);
    TEST_ASSERT_EQUAL(7, elem2->as.int32);
    TEST_ASSERT_EQUAL(10, elem3->as.int32);
    vm_release(result);
}

// Test stepped range class methods
void test_stepped_range_methods(void) {
    // Test length() with stepped ranges
    value_t result = test_execute_expression("(1..10 step 2).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32); // [1, 3, 5, 7, 9]
    vm_release(result);
    
    result = test_execute_expression("(10..1 step -3).length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(4, result.as.int32); // [10, 7, 4, 1]
    vm_release(result);
    
    // Test contains() with step alignment
    result = test_execute_expression("(1..10 step 2).contains(3)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // 3 is reachable
    vm_release(result);
    
    result = test_execute_expression("(1..10 step 2).contains(4)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // 4 is not reachable
    vm_release(result);
}

// Test exclusive stepped ranges
void test_exclusive_stepped_ranges(void) {
    // Test 1..<10 step 2 -> [1, 3, 5, 7, 9] (9 < 10 so included)
    value_t result = test_execute_expression("(1..<10 step 2).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    value_t* elem4 = (value_t*)da_get(result.as.array, 4);
    TEST_ASSERT_EQUAL(9, elem4->as.int32); // Last element should be 9, not 10
    vm_release(result);
    
    // Test 1..<11 step 2 -> [1, 3, 5, 7, 9] (would reach 11 but it's excluded)
    result = test_execute_expression("(1..<11 step 2).toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array)); // Same as above - 11 is excluded
    vm_release(result);
}

// Test error conditions for stepped ranges
void test_stepped_range_errors(void) {
    // Test zero step error
    bool error_occurred = test_expect_error("1..10 step 0", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
    
    // Test wrong direction error  
    error_occurred = test_expect_error("1..10 step -1", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
    
    // Test another wrong direction error
    error_occurred = test_expect_error("10..1 step 2", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

void test_stepped_ranges_suite(void) {
    RUN_TEST(test_stepped_range_construction);
    RUN_TEST(test_stepped_range_iterator);
    RUN_TEST(test_stepped_range_manual_iterator);
    RUN_TEST(test_stepped_range_methods);
    RUN_TEST(test_exclusive_stepped_ranges);
    RUN_TEST(test_stepped_range_errors);
}