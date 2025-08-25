#include "unity.h"
#include "vm.h"
#include "builtins.h"
#include "parser.h"
#include "codegen.h"
#include "lexer.h"

// Helper function to interpret a single expression and return result
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
    
    codegen_t* codegen = codegen_create();
    function_t* function = codegen_compile(codegen, program);
    
    bitty_vm* vm = vm_create();
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


// Test print function (returns null)
void test_builtin_print(void) {
    value_t result = interpret_expression("print(42)");
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test type function for numeric types
void test_builtin_type_number(void) {
    // Test int32 type
    value_t result = interpret_expression("type(42)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("int32", result.as.string);
    vm_release(result);
    
    // Test number (float) type
    result = interpret_expression("type(3.14)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("number", result.as.string);
    vm_release(result);
}

void test_builtin_type_string(void) {
    value_t result = interpret_expression("type(\"hello\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("string", result.as.string);
    vm_release(result);
}

void test_builtin_type_boolean(void) {
    value_t result = interpret_expression("type(true)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("boolean", result.as.string);
    vm_release(result);
}

void test_builtin_type_null(void) {
    value_t result = interpret_expression("type(null)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null", result.as.string);
    vm_release(result);
}

// Test abs function
void test_builtin_abs_positive(void) {
    value_t result = interpret_expression("abs(5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_negative(void) {
    value_t result = interpret_expression("abs(-5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_zero(void) {
    value_t result = interpret_expression("abs(0)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
}

// Test sqrt function
void test_builtin_sqrt(void) {
    value_t result = interpret_expression("sqrt(16)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, result.as.number);
    vm_release(result);
}

void test_builtin_sqrt_zero(void) {
    value_t result = interpret_expression("sqrt(0)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);
    vm_release(result);
}

// Test floor function
void test_builtin_floor(void) {
    value_t result = interpret_expression("floor(3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_floor_negative(void) {
    value_t result = interpret_expression("floor(-3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-4, result.as.int32);
    vm_release(result);
}

// Test ceil function
void test_builtin_ceil(void) {
    value_t result = interpret_expression("ceil(3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_ceil_negative(void) {
    value_t result = interpret_expression("ceil(-3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-3, result.as.int32);
    vm_release(result);
}

// Test round function
void test_builtin_round_up(void) {
    value_t result = interpret_expression("round(3.6)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_round_down(void) {
    value_t result = interpret_expression("round(3.4)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_round_half(void) {
    value_t result = interpret_expression("round(3.5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

// Test min function
void test_builtin_min(void) {
    value_t result = interpret_expression("min(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_min_negative(void) {
    value_t result = interpret_expression("min(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-5, result.as.int32);
    vm_release(result);
}

// Test max function
void test_builtin_max(void) {
    value_t result = interpret_expression("max(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);
}

void test_builtin_max_negative(void) {
    value_t result = interpret_expression("max(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-2, result.as.int32);
    vm_release(result);
}

// Test random function (just check it returns a number in valid range)
void test_builtin_random(void) {
    value_t result = interpret_expression("random()");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_TRUE(result.as.number >= 0.0);
    TEST_ASSERT_TRUE(result.as.number <= 1.0);
    vm_release(result);
}

// Test suite function
void test_builtins_suite(void) {
    RUN_TEST(test_builtin_print);
    RUN_TEST(test_builtin_type_number);
    RUN_TEST(test_builtin_type_string);
    RUN_TEST(test_builtin_type_boolean);
    RUN_TEST(test_builtin_type_null);
    RUN_TEST(test_builtin_abs_positive);
    RUN_TEST(test_builtin_abs_negative);
    RUN_TEST(test_builtin_abs_zero);
    RUN_TEST(test_builtin_sqrt);
    RUN_TEST(test_builtin_sqrt_zero);
    RUN_TEST(test_builtin_floor);
    RUN_TEST(test_builtin_floor_negative);
    RUN_TEST(test_builtin_ceil);
    RUN_TEST(test_builtin_ceil_negative);
    RUN_TEST(test_builtin_round_up);
    RUN_TEST(test_builtin_round_down);
    RUN_TEST(test_builtin_round_half);
    RUN_TEST(test_builtin_min);
    RUN_TEST(test_builtin_min_negative);
    RUN_TEST(test_builtin_max);
    RUN_TEST(test_builtin_max_negative);
    RUN_TEST(test_builtin_random);
}