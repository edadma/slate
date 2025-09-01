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
// OBJECT CLASS BASIC TESTS
// ===========================

// Test object construction
void test_object_construction(void) {
    // Test empty object
    value_t result = run_code("{}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
    
    // Test object with properties
    result = run_code("{x: 42, y: \"hello\"}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// Test object type checking
void test_object_type_checking(void) {
    value_t result = run_code("type({})");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("object", result.as.string);
    vm_release(result);
    
    result = run_code("type({x: 42})");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("object", result.as.string);
    vm_release(result);
}

// Test object property access
void test_object_property_access(void) {
    // Test property access with run_code (simpler than variables)
    value_t result = run_code("{x: 42, y: \"hello\"}.x");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
    
    result = run_code("{x: 42, y: \"hello\"}.y");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
}

// ===========================
// OBJECT DISPLAY AND CONVERSION TESTS  
// ===========================

// Test object with string values (moved from test_builtins.c)
void test_object_with_string_values(void) {
    value_t result = interpret_expression("{greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    // Convert to string to check display format
    value_t str_result = interpret_expression("\"\" + {greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_STRING, str_result.type);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "greeting: \"hello\"") != NULL);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "name: \"world\"") != NULL);
    vm_release(result);
    vm_release(str_result);
}

// Test object string conversion
void test_object_string_conversion(void) {
    // Test empty object string conversion
    value_t result = run_code("\"\" + {}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "{}") != NULL);
    vm_release(result);
    
    // Test object with number properties
    result = run_code("\"\" + {x: 1, y: 2}");
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
    value_t result = run_code("{num: 42, str: \"hello\", bool: true, null_val: null}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// Test nested objects
void test_object_nested(void) {
    value_t result = run_code("{outer: {inner: 42}}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
    
    // Test nested property access
    result = run_code("{outer: {inner: 42}}.outer.inner");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test object edge cases
void test_object_edge_cases(void) {
    // Test object with array property
    value_t result = run_code("{arr: [1, 2, 3]}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
    
    // Test object with range property
    result = run_code("{range: 1..5}");
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