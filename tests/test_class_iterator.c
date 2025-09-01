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

// Helper function to interpret a single expression and return result (from test_builtins.c)
static value_t interpret_expression(const char* source) {
    lexer_t lexer;
    parser_t parser;
    
    lexer_init(&lexer, source);
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
        // Retain result before cleanup
        return_value = vm_retain(return_value);
    }
    
    // Cleanup (don't call function_destroy - codegen owns the function)
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return return_value;
}

// ===========================
// ITERATOR CLASS BASIC TESTS
// ===========================

// Test iterator creation from arrays
void test_iterator_creation_from_array(void) {
    value_t result = run_code("[1, 2, 3].iterator()");
    TEST_ASSERT_EQUAL(VAL_ITERATOR, result.type);
    TEST_ASSERT_NOT_NULL(result.as.iterator);
    vm_release(result);
}

// Test iterator creation from ranges
void test_iterator_creation_from_range(void) {
    value_t result = run_code("(1..5).iterator()");
    TEST_ASSERT_EQUAL(VAL_ITERATOR, result.type);
    TEST_ASSERT_NOT_NULL(result.as.iterator);
    vm_release(result);
}

// Test iterator type checking
void test_iterator_type_checking(void) {
    value_t result = run_code("type([1, 2].iterator())");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("iterator", result.as.string);
    vm_release(result);
}

// ===========================
// ITERATOR METHOD TESTS
// ===========================

void test_iterator_has_next_next(void) {
    // Test basic iterator functionality with array
    value_t result = interpret_expression("var it = [1, 2].iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true
    vm_release(result);
    
    result = interpret_expression("var it = [1, 2].iterator(); it.next()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32); // first element
    vm_release(result);
    
    // Test empty iterator
    result = interpret_expression("var it = [].iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false
    vm_release(result);
    
    // Test range iterator
    result = interpret_expression("var it = (1..2).iterator(); it.hasNext()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true
    vm_release(result);
    
    result = interpret_expression("var it = (1..2).iterator(); it.next()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1, result.as.int32); // first element
    vm_release(result);
}

void test_iterator_is_empty(void) {
    // Test empty iterator
    value_t result = interpret_expression("var it = [].iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true - empty
    vm_release(result);
    
    // Test non-empty iterator  
    result = interpret_expression("var it = [1, 2, 3].iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false - not empty
    vm_release(result);
    
    // Test range iterator
    result = interpret_expression("var it = (1..3).iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean); // false - not empty
    vm_release(result);
    
    // Test empty range iterator  
    result = interpret_expression("var it = (5..<5).iterator(); it.isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean); // true - empty
    vm_release(result);
}

void test_iterator_to_array(void) {
    // Test array iterator to array
    value_t result = interpret_expression("var it = [1, 2, 3].iterator(); it.toArray()");
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
    
    // Test range iterator to array
    result = interpret_expression("var it = (1..3).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    
    elem0 = (value_t*)da_get(result.as.array, 0);
    elem1 = (value_t*)da_get(result.as.array, 1);
    elem2 = (value_t*)da_get(result.as.array, 2);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    TEST_ASSERT_EQUAL(2, elem1->as.int32);
    TEST_ASSERT_EQUAL(3, elem2->as.int32);
    vm_release(result);
    
    // Test empty iterator to array
    result = interpret_expression("var it = [].iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(0, da_length(result.as.array)); // empty array
    vm_release(result);
}

void test_iterator_array_vs_range(void) {
    // Test that array and range iterators behave the same
    value_t array_result = interpret_expression("[1, 2, 3].iterator().toArray()");
    value_t range_result = interpret_expression("(1..3).iterator().toArray()");
    
    TEST_ASSERT_EQUAL(VAL_ARRAY, array_result.type);
    TEST_ASSERT_EQUAL(VAL_ARRAY, range_result.type);
    TEST_ASSERT_EQUAL(da_length(array_result.as.array), da_length(range_result.as.array));
    
    // Compare elements
    for (int i = 0; i < (int)da_length(array_result.as.array); i++) {
        value_t* arr_elem = (value_t*)da_get(array_result.as.array, i);
        value_t* range_elem = (value_t*)da_get(range_result.as.array, i);
        TEST_ASSERT_EQUAL(arr_elem->as.int32, range_elem->as.int32);
    }
    
    vm_release(array_result);
    vm_release(range_result);
    
    // Test isEmpty consistency
    value_t array_empty = interpret_expression("[].iterator().isEmpty()");
    value_t range_empty = interpret_expression("(5..<5).iterator().isEmpty()");
    
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, array_empty.type);
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, range_empty.type);
    TEST_ASSERT_EQUAL(1, array_empty.as.boolean);  // both empty
    TEST_ASSERT_EQUAL(1, range_empty.as.boolean);  // both empty
    
    vm_release(array_empty);
    vm_release(range_empty);
}

// ===========================
// ITERATOR COMPREHENSIVE TESTS
// ===========================

// Test iterator state progression
void test_iterator_state_progression(void) {
    // Test array iterator progression
    value_t result = interpret_expression("var it = [42, 99].iterator(); var first = it.next(); var has_next = it.hasNext(); var second = it.next(); var done = it.hasNext(); first");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test iterator edge cases
void test_iterator_edge_cases(void) {
    // Test single element iterator
    value_t result = interpret_expression("var it = [42].iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array));
    
    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(42, elem0->as.int32);
    vm_release(result);
    
    // Test exclusive range iterator
    result = interpret_expression("var it = (1..<2).iterator(); it.toArray()");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(1, da_length(result.as.array)); // only element 1
    
    elem0 = (value_t*)da_get(result.as.array, 0);
    TEST_ASSERT_EQUAL(1, elem0->as.int32);
    vm_release(result);
}

// Test iterator method chaining (via conversion)
void test_iterator_method_chaining(void) {
    // Convert iterator to array then back to iterator
    value_t result = interpret_expression("var arr = [1, 2, 3].iterator().toArray(); arr");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL(3, da_length(result.as.array));
    vm_release(result);
}

// Test Suite Runner
void test_class_iterator_suite(void) {
    RUN_TEST(test_iterator_creation_from_array);
    RUN_TEST(test_iterator_creation_from_range);
    RUN_TEST(test_iterator_type_checking);
    RUN_TEST(test_iterator_has_next_next);
    RUN_TEST(test_iterator_is_empty);
    RUN_TEST(test_iterator_to_array);
    RUN_TEST(test_iterator_array_vs_range);
    RUN_TEST(test_iterator_state_progression);
    RUN_TEST(test_iterator_edge_cases);
    RUN_TEST(test_iterator_method_chaining);
}