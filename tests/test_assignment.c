#include <math.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source);

// Test compound assignment operators
void test_vm_compound_assignments(void) {
    value_t result;
    // Test += with integers
    result = run_code("var x = 10; x += 5; x");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
    vm_release(result);
    // Test -= with integers
    result = run_code("var y = 20; y -= 8; y");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32);
    vm_release(result);
    // Test *= with integers
    result = run_code("var z = 4; z *= 3; z");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32);
    vm_release(result);
    // Test /= with integers (should result in float)
    result = run_code("var w = 15; w /= 3; w");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.number);
    vm_release(result);
    // Test %= with integers
    result = run_code("var m = 17; m %= 5; m");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
    vm_release(result);
    // Test **= with integers (power always returns float)
    result = run_code("var p = 3; p **= 2; p");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(9.0, result.as.number);
    vm_release(result);
    // Test chained compound assignments
    result = run_code("var chain = 2; chain *= 3; chain += 1; chain");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);
}

// Test new compound assignment operators
void test_vm_new_compound_assignments(void) {
    value_t result;
    
    // Test &= with integers
    result = run_code("var x = 12; x &= 10; x");  // 12 & 10 = 8
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32);
    vm_release(result);
    
    // Test |= with integers
    result = run_code("var y = 12; y |= 3; y");  // 12 | 3 = 15
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
    vm_release(result);
    
    // Test ^= with integers
    result = run_code("var z = 12; z ^= 10; z");  // 12 ^ 10 = 6
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);
    vm_release(result);
    
    // Test &&= with truthy values
    result = run_code("var a = 5; a &&= 7; a");  // 5 && 7 = 7
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);
    
    // Test &&= with falsy values (short-circuiting)
    result = run_code("var b = 0; b &&= 42; b");  // 0 && 42 = 0
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
    
    // Test ||= with falsy values
    result = run_code("var c = 0; c ||= 42; c");  // 0 || 42 = 42
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
    
    // Test ||= with truthy values (short-circuiting)
    result = run_code("var d = 5; d ||= 99; d");  // 5 || 99 = 5
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
    
    // Test bitwise operators with different bit patterns
    result = run_code("var bits = 10; bits &= 12; bits");  // 10 & 12 = 8
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32);
    vm_release(result);
}

// Assignment test suite
void test_assignment_suite(void) {
    RUN_TEST(test_vm_compound_assignments);
    RUN_TEST(test_vm_new_compound_assignments);
}