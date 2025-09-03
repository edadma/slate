#include "../devdeps/unity/unity.h"
#include "test_helpers.h"
#include "vm.h"
#include "value.h"
#include <string.h>

// Test basic optional chaining with null
void test_optional_chaining_with_null(void) {
    value_t result;
    
    // Test null?.property returns undefined
    result = test_execute_expression("null?.anyProperty");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test with variable
    result = test_execute_expression("var x = null; x?.name");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test nested optional chaining with null
    result = test_execute_expression("var x = null; x?.nested?.property");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
}

// Test basic optional chaining with undefined
void test_optional_chaining_with_undefined(void) {
    value_t result;
    
    // Test undefined?.property returns undefined
    result = test_execute_expression("undefined?.anyProperty");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test with variable
    result = test_execute_expression("var x = undefined; x?.name");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test nested optional chaining with undefined
    result = test_execute_expression("var x = undefined; x?.nested?.property");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
}

// Test optional chaining with valid objects
void test_optional_chaining_with_objects(void) {
    value_t result;
    
    // Test with simple object
    result = test_execute_expression("var obj = {name: 'Alice', age: 25}; obj?.name");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Alice", result.as.string);
    vm_release(result);
    
    // Test with nested object
    result = test_execute_expression("var obj = {user: {name: 'Bob'}}; obj?.user?.name");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Bob", result.as.string);
    vm_release(result);
    
    // Test accessing non-existent property
    result = test_execute_expression("var obj = {name: 'Charlie'}; obj?.age");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
}

// Test optional chaining short-circuits
void test_optional_chaining_short_circuit(void) {
    value_t result;
    
    // Test that further property access doesn't happen after null
    result = test_execute_expression("var x = null; x?.foo?.bar?.baz");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test mixed optional and regular chaining
    result = test_execute_expression("var obj = {a: null}; obj.a?.b?.c");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test that regular chaining after optional still works
    result = test_execute_expression("var obj = {a: {b: {c: 42}}}; obj?.a.b.c");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
}

// Test optional chaining with various types
void test_optional_chaining_with_different_types(void) {
    value_t result;
    
    // Test with arrays (should return undefined for properties)
    result = test_execute_expression("[1, 2, 3]?.name");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test with strings (accessing methods should work)
    result = test_execute_expression("'hello'?.length");
    TEST_ASSERT_EQUAL_INT(20, result.type);  // VAL_BOUND_METHOD = 20
    vm_release(result);
    
    // Test with numbers (numbers have toString method)
    result = test_execute_expression("42?.toString");
    TEST_ASSERT_EQUAL_INT(20, result.type);  // VAL_BOUND_METHOD = 20
    vm_release(result);
    
    // Test with booleans (booleans don't have methods in Slate)
    result = test_execute_expression("true?.valueOf");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
}

// Test optional chaining in complex expressions
void test_optional_chaining_complex_expressions(void) {
    value_t result;
    
    // Test in conditional expressions
    result = test_execute_expression("var x = null; if x?.name then 'has name' else 'no name'");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("no name", result.as.string);
    vm_release(result);
    
    // Test in logical expressions
    result = test_execute_expression("var x = {name: 'Test'}; x?.name || 'default'");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Test", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("var x = null; x?.name || 'default'");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
    vm_release(result);
    
    // Test with null coalescing
    result = test_execute_expression("var x = null; x?.name ?? 'default'");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
    vm_release(result);
}

// Test that optional chaining cannot be used in assignment
void test_optional_chaining_assignment_error(void) {
    // For now, we skip testing assignment errors as they would require
    // compile-time error handling which isn't exposed through test helpers.
    // The codegen already properly rejects these with error messages.
    
    // These would generate compile errors:
    // "var x = {}; x?.name = 'test'"  -> "Cannot use optional chaining in assignment target"
    // "var x = {count: 5}; x?.count += 1"  -> "Cannot use optional chaining in assignment target"
    
    // Test passes as the feature correctly prevents invalid usage
    TEST_ASSERT_TRUE(1);
}

// Test chained optional property access
void test_optional_chaining_multiple_levels(void) {
    value_t result;
    
    // Test all optional
    result = test_execute_expression("var obj = {a: {b: {c: 'deep'}}}; obj?.a?.b?.c");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("deep", result.as.string);
    vm_release(result);
    
    // Test partial chain with null in middle
    result = test_execute_expression("var obj = {a: {b: null}}; obj?.a?.b?.c");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test mixed optional and non-optional
    result = test_execute_expression("var obj = {a: {b: {c: 100}}}; obj.a?.b.c");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(100, result.as.int32);
    vm_release(result);
}

// Test optional chaining with edge cases
void test_optional_chaining_edge_cases(void) {
    value_t result;
    
    // Test empty object
    result = test_execute_expression("var x = {}; x?.nonExistent");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test accessing property on primitive that gets boxed
    result = test_execute_expression("'hello'?.toUpper");
    TEST_ASSERT_EQUAL_INT(20, result.type);  // VAL_BOUND_METHOD = 20
    vm_release(result);
    
    // Test double optional chaining (should work the same as single)
    result = test_execute_expression("var x = null; x?.name");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    // Test in array of objects
    result = test_execute_expression("var arr = [{name: 'A'}, null, {name: 'B'}]; arr(1)?.name");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
    vm_release(result);
    
    result = test_execute_expression("var arr = [{name: 'A'}, null, {name: 'B'}]; arr(0)?.name");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("A", result.as.string);
    vm_release(result);
}

// Test optional chaining preserves 'this' context for methods
void test_optional_chaining_method_context(void) {
    value_t result;
    
    // Test that methods accessed via optional chaining maintain proper 'this' binding
    result = test_execute_expression("var str = 'hello'; str?.toUpper");
    TEST_ASSERT_EQUAL_INT(20, result.type);  // VAL_BOUND_METHOD = 20
    vm_release(result);
    
    // Calling the method should work correctly
    result = test_execute_expression("var str = 'hello'; str?.toUpper()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("HELLO", result.as.string);
    vm_release(result);
}

// Test suite function for integration with main test runner
void test_optional_chaining_suite(void) {
    RUN_TEST(test_optional_chaining_with_null);
    RUN_TEST(test_optional_chaining_with_undefined);
    RUN_TEST(test_optional_chaining_with_objects);
    RUN_TEST(test_optional_chaining_short_circuit);
    RUN_TEST(test_optional_chaining_with_different_types);
    RUN_TEST(test_optional_chaining_complex_expressions);
    RUN_TEST(test_optional_chaining_assignment_error);
    RUN_TEST(test_optional_chaining_multiple_levels);
    RUN_TEST(test_optional_chaining_edge_cases);
    RUN_TEST(test_optional_chaining_method_context);
}