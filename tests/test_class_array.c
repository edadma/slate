#include "../unity/unity.h"
#include "../include/vm.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/builtins.h"
#include <string.h>

// Helper function to run code and get result
static value_t run_code(const char* code) {
    lexer_t lexer;
    parser_t parser;
    
    lexer_init(&lexer, code);
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return make_null();
    }
    
    slate_vm* vm = vm_create();
    builtins_init(vm);
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    
    vm_result result = vm_execute(vm, function);
    
    value_t return_value = make_null();
    if (result == VM_OK) {
        return_value = vm->result;
        // Retain the result to survive cleanup
        return_value = vm_retain(return_value);
    }
    
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return return_value;
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

// Test Array(1..5) - inclusive range constructor
void test_array_constructor_inclusive_range(void) {
    value_t result = run_code("Array(1..5)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements: [1, 2, 3, 4, 5]
    for (int i = 0; i < 5; i++) {
        value_t* elem = (value_t*)da_get(result.as.array, i);
        TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
        TEST_ASSERT_EQUAL(i + 1, elem->as.int32);
    }
    
    vm_release(result);
}

// Test Array(1..<5) - exclusive range constructor
void test_array_constructor_exclusive_range(void) {
    value_t result = run_code("Array(1..<5)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(4, da_length(result.as.array));
    
    // Check elements: [1, 2, 3, 4]
    for (int i = 0; i < 4; i++) {
        value_t* elem = (value_t*)da_get(result.as.array, i);
        TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
        TEST_ASSERT_EQUAL(i + 1, elem->as.int32);
    }
    
    vm_release(result);
}

// Test Array(5..1) - reverse range constructor
void test_array_constructor_reverse_range(void) {
    value_t result = run_code("Array(5..1)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(5, da_length(result.as.array));
    
    // Check elements: [5, 4, 3, 2, 1]
    for (int i = 0; i < 5; i++) {
        value_t* elem = (value_t*)da_get(result.as.array, i);
        TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
        TEST_ASSERT_EQUAL(5 - i, elem->as.int32);
    }
    
    vm_release(result);
}

// Test Array(5..<1) - reverse exclusive range constructor
void test_array_constructor_reverse_exclusive_range(void) {
    value_t result = run_code("Array(5..<1)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(4, da_length(result.as.array));
    
    // Check elements: [5, 4, 3, 2]
    for (int i = 0; i < 4; i++) {
        value_t* elem = (value_t*)da_get(result.as.array, i);
        TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
        TEST_ASSERT_EQUAL(5 - i, elem->as.int32);
    }
    
    vm_release(result);
}

// Test Array(0..0) - single element range
void test_array_constructor_single_element_range(void) {
    value_t result = run_code("Array(0..0)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_NOT_NULL(result.as.array);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array));
    
    value_t* elem = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(VAL_INT32, elem->type);
    TEST_ASSERT_EQUAL(0, elem->as.int32);
    
    vm_release(result);
}

// Test Array(0..<0) - empty exclusive range
void test_array_constructor_empty_exclusive_range(void) {
    value_t result = run_code("Array(0..<0)");
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

// Test Array fill method exists
void test_array_fill_method_exists(void) {
    // Test that fill method exists on arrays and returns a bound method
    value_t result = run_code("Array().fill");
    TEST_ASSERT_EQUAL(VAL_BOUND_METHOD, result.type);
    vm_release(result);
}

// Test Array fill method with zero size and null function
void test_array_fill_zero_size(void) {
    // Test zero size - should create empty array regardless of function validity
    value_t result = run_code("Array().fill(0, null)");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array));
    vm_release(result);
}

// Note: Full functional tests for Array.fill with working functions will be 
// enabled once function compilation is fully implemented. The method exists,
// validates arguments correctly, and will work once functions are complete.

// Test Suite Runner
void test_class_array_suite(void) {
    RUN_TEST(test_array_constructor_empty);
    RUN_TEST(test_array_constructor_multiple_args);
    RUN_TEST(test_array_constructor_copy_array);
    RUN_TEST(test_array_constructor_copy_empty_array);
    RUN_TEST(test_array_constructor_inclusive_range);
    RUN_TEST(test_array_constructor_exclusive_range);
    RUN_TEST(test_array_constructor_reverse_range);
    RUN_TEST(test_array_constructor_reverse_exclusive_range);
    RUN_TEST(test_array_constructor_single_element_range);
    RUN_TEST(test_array_constructor_empty_exclusive_range);
    RUN_TEST(test_array_constructor_single_element);
    RUN_TEST(test_array_constructor_single_number);
    RUN_TEST(test_array_constructor_mixed_types);
    RUN_TEST(test_array_constructor_independence);
    RUN_TEST(test_array_constructor_methods);
    RUN_TEST(test_array_constructor_with_push);
    RUN_TEST(test_array_constructor_method_chaining);
    RUN_TEST(test_array_fill_method_exists);
    RUN_TEST(test_array_fill_zero_size);
}