#include "unity.h"
#include "vm.h"
#include "codegen.h"
#include "parser.h"
#include "lexer.h"
#include <string.h>
#include <math.h>

// Helper function to compile and run code
bit_value run_code(const char* source) {
    lexer_t lexer;
    parser_t parser;
    
    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        return make_null();
    }
    
    codegen_t* codegen = codegen_create();
    bit_function* function = codegen_compile(codegen, program);
    
    bit_vm* vm = vm_create();
    vm_result result = vm_execute(vm, function);
    
    bit_value return_value = make_null();
    if (result == VM_OK && vm->stack_top > vm->stack) {
        return_value = vm->stack[0];
        // Retain strings to survive cleanup
        if (return_value.type == VAL_STRING) {
            return_value.as.string = ds_retain(return_value.as.string);
        }
    }
    
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    
    return return_value;
}

// Test numeric operations
void test_vm_arithmetic(void) {
    bit_value result;
    
    result = run_code("42");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(42.0, result.as.number);
    
    result = run_code("2 + 3");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.number);
    
    result = run_code("10 - 4");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(6.0, result.as.number);
    
    result = run_code("3 * 7");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(21.0, result.as.number);
    
    result = run_code("15 / 3");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.number);
    
    result = run_code("2 + 3 * 4");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(14.0, result.as.number);
    
    result = run_code("(2 + 3) * 4");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(20.0, result.as.number);
}

// Test unary operations
void test_vm_unary(void) {
    bit_value result;
    
    result = run_code("-42");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(-42.0, result.as.number);
    
    result = run_code("3 + -4");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(-1.0, result.as.number);
    
    result = run_code("-(-5)");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.number);
}

// Test string operations
void test_vm_strings(void) {
    bit_value result;
    
    result = run_code("\"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    ds_release(&result.as.string);
    
    result = run_code("\"hello\" + \" world\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello world", result.as.string);
    ds_release(&result.as.string);
    
    result = run_code("\"Aug \" + 23");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Aug 23", result.as.string);
    ds_release(&result.as.string);
    
    result = run_code("42 + \" is the answer\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("42 is the answer", result.as.string);
    ds_release(&result.as.string);
}

// Test boolean operations
void test_vm_booleans(void) {
    bit_value result;
    
    result = run_code("true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    
    result = run_code("false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    
    result = run_code("true + \" or false\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("true or false", result.as.string);
    ds_release(&result.as.string);
}

// Test null
void test_vm_null(void) {
    bit_value result;
    
    result = run_code("null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    
    result = run_code("null + \" value\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null value", result.as.string);
    ds_release(&result.as.string);
}

// Test value creation functions
void test_vm_value_creation(void) {
    bit_value val;
    
    val = make_null();
    TEST_ASSERT_EQUAL_INT(VAL_NULL, val.type);
    
    val = make_boolean(1);
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, val.type);
    TEST_ASSERT_EQUAL_INT(1, val.as.boolean);
    
    val = make_boolean(0);
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, val.type);
    TEST_ASSERT_EQUAL_INT(0, val.as.boolean);
    
    val = make_number(3.14);
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, val.type);
    TEST_ASSERT_EQUAL_DOUBLE(3.14, val.as.number);
    
    val = make_string("test");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, val.type);
    TEST_ASSERT_EQUAL_STRING("test", val.as.string);
    free_value(val);
}

// Test value comparison
void test_vm_value_equality(void) {
    bit_value a, b;
    
    // Numbers
    a = make_number(42);
    b = make_number(42);
    TEST_ASSERT_TRUE(values_equal(a, b));
    
    b = make_number(43);
    TEST_ASSERT_FALSE(values_equal(a, b));
    
    // Strings
    a = make_string("hello");
    b = make_string("hello");
    TEST_ASSERT_TRUE(values_equal(a, b));
    free_value(a);
    free_value(b);
    
    a = make_string("hello");
    b = make_string("world");
    TEST_ASSERT_FALSE(values_equal(a, b));
    free_value(a);
    free_value(b);
    
    // Booleans
    a = make_boolean(1);
    b = make_boolean(1);
    TEST_ASSERT_TRUE(values_equal(a, b));
    
    b = make_boolean(0);
    TEST_ASSERT_FALSE(values_equal(a, b));
    
    // Null
    a = make_null();
    b = make_null();
    TEST_ASSERT_TRUE(values_equal(a, b));
    
    // Different types
    a = make_number(42);
    b = make_string("42");
    TEST_ASSERT_FALSE(values_equal(a, b));
    free_value(b);
}

// Test is_falsy
void test_vm_is_falsy(void) {
    TEST_ASSERT_TRUE(is_falsy(make_null()));
    TEST_ASSERT_TRUE(is_falsy(make_boolean(0)));
    TEST_ASSERT_FALSE(is_falsy(make_boolean(1)));
    TEST_ASSERT_FALSE(is_falsy(make_number(0)));
    TEST_ASSERT_FALSE(is_falsy(make_number(42)));
    
    bit_value str = make_string("");
    TEST_ASSERT_FALSE(is_falsy(str));
    free_value(str);
}

// Test suite runner
void test_vm_suite(void) {
    RUN_TEST(test_vm_arithmetic);
    RUN_TEST(test_vm_unary);
    RUN_TEST(test_vm_strings);
    RUN_TEST(test_vm_booleans);
    RUN_TEST(test_vm_null);
    RUN_TEST(test_vm_value_creation);
    RUN_TEST(test_vm_value_equality);
    RUN_TEST(test_vm_is_falsy);
}