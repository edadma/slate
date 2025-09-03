#include <limits.h>
#include <stdio.h>
#include "ast.h"
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to compile and execute a source string, returning the result value
static value_t execute_expression(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    TEST_ASSERT_FALSE(parser.had_error);
    TEST_ASSERT_NOT_NULL(program);

    vm_t* vm = vm_create();
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    TEST_ASSERT_FALSE(codegen->had_error);
    TEST_ASSERT_NOT_NULL(function);

    vm_result result = vm_execute(vm, function);
    TEST_ASSERT_EQUAL(VM_OK, result);

    // Copy the result (simple copy for int32, retain for reference-counted types)
    value_t ret_value = vm->result;
    if (ret_value.type == VAL_BIGINT) {
        ret_value.as.bigint = di_retain(ret_value.as.bigint);
    }

    // Cleanup (function already destroyed by VM during OP_HALT)
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return ret_value;
}

// Helper function that allows execution failures (for error testing)
static value_t execute_expression_allow_errors(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return make_null();
    }

    vm_t* vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);

    value_t return_value = make_null();
    
    // Use setjmp to catch errors from runtime_error
    if (setjmp(vm->trap) == 0) {
        vm_result result = vm_execute(vm, function);
        if (result == VM_OK) {
            return_value = vm->result;
            // Retain strings and other reference-counted types to survive cleanup
            return_value = vm_retain(return_value);
        }
    } else {
        // Error occurred - check vm->error for details if needed
        // For now, return null as before (tests expect null on error)
        return_value = make_null();
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return return_value;
}

void test_basic_int32_arithmetic() {
    // Basic int32 addition (no overflow)
    value_t result = execute_expression("100 + 200");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(300, result.as.int32);
    vm_release(result);

    // int32 multiplication
    result = execute_expression("50 * 20");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1000, result.as.int32);
    vm_release(result);

    // int32 subtraction
    result = execute_expression("1000 - 250");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(750, result.as.int32);
    vm_release(result);

    // int32 modulo
    result = execute_expression("17 mod 5");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
    vm_release(result);
}

void test_int32_division_always_float() {
    // Division always produces float, even for exact divisions
    value_t result = execute_expression("15 / 3");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.float64);
    vm_release(result);

    // Non-exact division
    result = execute_expression("7 / 2");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(3.5, result.as.float64);
    vm_release(result);
}

void test_int32_overflow_promotion() {
    // Addition overflow - should promote to BigInt
    char overflow_add[64];
    sprintf(overflow_add, "%d + 1000", INT32_MAX - 500);
    value_t result = execute_expression(overflow_add);
    TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
    vm_release(result);

    // Multiplication overflow - should promote to BigInt
    result = execute_expression("100000 * 50000");
    TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
    vm_release(result);

    // Subtraction overflow (underflow) - should promote to BigInt
    sprintf(overflow_add, "%d - 1000", INT32_MIN + 500);
    result = execute_expression(overflow_add);
    TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
    vm_release(result);
}

void test_mixed_int_float_arithmetic() {
    // int32 + float -> float
    value_t result = execute_expression("42 + 3.14");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(45.14, result.as.float64);
    vm_release(result);

    // float + int32 -> float
    result = execute_expression("3.14 + 42");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(45.14, result.as.float64);
    vm_release(result);

    // int32 * float -> float
    result = execute_expression("5 * 2.5");
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(12.5, result.as.float64);
    vm_release(result);
}

void test_operator_precedence_with_integers() {
    // Multiplication before addition
    value_t result = execute_expression("2 + 3 * 4");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32);
    vm_release(result);

    // Parentheses override precedence
    result = execute_expression("(2 + 3) * 4");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);
    vm_release(result);

    // Complex precedence
    result = execute_expression("10 - 2 * 3 + 1");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32); // 10 - 6 + 1 = 5
    vm_release(result);
}

void test_unary_arithmetic() {
    // Unary minus on int32
    value_t result = execute_expression("-42");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-42, result.as.int32);
    vm_release(result);

    // Unary minus on positive expression
    result = execute_expression("-(5 + 3)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-8, result.as.int32);
    vm_release(result);

    // Double negation (use spaces to avoid parsing as decrement)
    result = execute_expression("- -42");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);
}

void test_large_arithmetic() {
    // Large number arithmetic should work seamlessly
    // This creates a BigInt during parsing
    char large_expr[128];
    sprintf(large_expr, "%lld + %lld", (long long)INT32_MAX + 1000LL, (long long)INT32_MAX + 2000LL);
    value_t result = execute_expression(large_expr);
    // Should now be BigInt (with enhanced BigInt literal parsing)
    TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
    vm_release(result);
}

// Test floor division operator
void test_floor_division() {
    value_t result;

    // Basic floor division with positive numbers
    result = execute_expression("17 // 3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);  // floor(17/3) = floor(5.666) = 5
    vm_release(result);

    result = execute_expression("20 // 3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);  // floor(20/3) = floor(6.666) = 6
    vm_release(result);

    // Floor division with negative numbers (towards negative infinity)
    result = execute_expression("-17 // 3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-6, result.as.int32);  // floor(-17/3) = floor(-5.666) = -6
    vm_release(result);

    result = execute_expression("17 // -3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-6, result.as.int32);  // floor(17/-3) = floor(-5.666) = -6
    vm_release(result);

    result = execute_expression("-17 // -3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);  // floor(-17/-3) = floor(5.666) = 5
    vm_release(result);

    // Floor division with floating point
    result = execute_expression("17.5 // 3.0");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);  // floor(17.5/3.0) = floor(5.833) = 5
    vm_release(result);

    // Exact division
    result = execute_expression("15 // 3");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test increment and decrement operators  
void test_increment_decrement() {
    // Test pre-increment operator
    {
        value_t result = execute_expression("var x = 5; ++x");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(6, result.as.int32);
        vm_release(result);
    }
    
    // Test pre-decrement operator
    {
        value_t result = execute_expression("var y = 10; --y");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(9, result.as.int32);
        vm_release(result);
    }
    
    // Test that variable is actually modified
    {
        value_t result = execute_expression("var z = 3; ++z; z");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
        vm_release(result);
    }
}

// Test comprehensive increment/decrement scenarios
void test_increment_decrement_comprehensive() {
    // Test increment with overflow to BigInt
    {
        value_t result = execute_expression("var x = 2147483647; ++x");
        TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
        char* str = di_to_string(result.as.bigint, 10);
        TEST_ASSERT_EQUAL_STRING("2147483648", str);
        free(str);
        vm_release(result);
    }
    
    // Test decrement with underflow to BigInt
    {
        value_t result = execute_expression("var x = -2147483648; --x");
        TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
        char* str = di_to_string(result.as.bigint, 10);
        TEST_ASSERT_EQUAL_STRING("-2147483649", str);
        free(str);
        vm_release(result);
    }
    
    // Test increment with floats
    {
        value_t result = execute_expression("var x = 3.14; ++x");
        TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
        TEST_ASSERT_EQUAL_DOUBLE(4.14, result.as.float64);
        vm_release(result);
    }
}

// Test that invalid increment/decrement operations are caught at compile time
void test_invalid_increment_decrement_errors() {
    // Create a temporary test program to check compilation errors
    parser_t parser;
    lexer_t lexer;
    vm_t* vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    
    codegen_t* codegen = codegen_create(vm);
    
    // Test 1: ++42 (increment on literal)
    lexer_init(&lexer, "++42");
    parser_init(&parser, &lexer);
    ast_program* program1 = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program1);
    
    function_t* func1 = codegen_compile(codegen, program1);
    TEST_ASSERT_TRUE(codegen->had_error);  // Should have compilation error
    TEST_ASSERT_NULL(func1);               // Should fail to compile
    
    ast_free((ast_node*)program1);
    codegen_destroy(codegen);
    vm_destroy(vm);
    
    // Test 2: --(-2147483648) (decrement on parenthesized literal)
    vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    codegen = codegen_create(vm);
    lexer_init(&lexer, "--(-2147483648)");
    parser_init(&parser, &lexer);
    ast_program* program2 = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program2);
    
    function_t* func2 = codegen_compile(codegen, program2);
    TEST_ASSERT_TRUE(codegen->had_error);  // Should have compilation error
    TEST_ASSERT_NULL(func2);               // Should fail to compile
    
    ast_free((ast_node*)program2);
    codegen_destroy(codegen);
    vm_destroy(vm);
    
    // Test 3: --(2 + 3) (decrement on expression)
    vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    codegen = codegen_create(vm);
    lexer_init(&lexer, "--(2 + 3)");
    parser_init(&parser, &lexer);
    ast_program* program3 = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program3);
    
    function_t* func3 = codegen_compile(codegen, program3);
    TEST_ASSERT_TRUE(codegen->had_error);  // Should have compilation error  
    TEST_ASSERT_NULL(func3);               // Should fail to compile
    
    ast_free((ast_node*)program3);
    codegen_destroy(codegen);
    vm_destroy(vm);
    
    // Test 4: ++3.14 (increment on float literal)
    vm = vm_create();
    vm->context = CTX_TEST;  // Set test context for silent error handling
    codegen = codegen_create(vm);
    lexer_init(&lexer, "++3.14");
    parser_init(&parser, &lexer);
    ast_program* program4 = parse_program(&parser);
    TEST_ASSERT_NOT_NULL(program4);
    
    function_t* func4 = codegen_compile(codegen, program4);
    TEST_ASSERT_TRUE(codegen->had_error);  // Should have compilation error
    TEST_ASSERT_NULL(func4);               // Should fail to compile
    
    ast_free((ast_node*)program4);
    codegen_destroy(codegen);
    vm_destroy(vm);
}

// Test comprehensive arithmetic operations (moved from test_vm.c)
void test_comprehensive_arithmetic() {
    value_t result;

    result = execute_expression("42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
    vm_release(result);

    result = execute_expression("2 + 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);

    result = execute_expression("10 - 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);
    vm_release(result);

    result = execute_expression("3 * 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(21, result.as.int32);
    vm_release(result);

    result = execute_expression("15 / 3");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(5.0, result.as.float64);
    vm_release(result);

    result = execute_expression("2 + 3 * 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32);
    vm_release(result);

    result = execute_expression("(2 + 3) * 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);
    vm_release(result);
}

// Test modulo operations (moved from test_vm.c)
void test_modulo_operations() {
    value_t result;
    
    // Test modulo operations
    result = execute_expression("10 mod 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
    vm_release(result);

    result = execute_expression("7 mod 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
    vm_release(result);

    result = execute_expression("100 mod 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
    vm_release(result);

    result = execute_expression("5 mod 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);

    result = execute_expression("4 mod 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);

    // Test floating point modulo
    result = execute_expression("15.5 mod 4.2");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.9, result.as.float64);
    vm_release(result);

    // Test modulo precedence (same as multiply/divide)
    result = execute_expression("10 + 7 mod 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(11, result.as.int32); // 10 + (7 mod 3) = 10 + 1 = 11
    vm_release(result);

    // Test mixed operator precedence with modulo
    result = execute_expression("2 * 5 mod 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32); // (2 * 5) mod 3 = 10 mod 3 = 1
    vm_release(result);

    result = execute_expression("15 mod 4 + 1");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32); // (15 mod 4) + 1 = 3 + 1 = 4
    vm_release(result);
}

// Test power operator (moved from test_vm.c)
void test_power_operations() {
    value_t result;
    
    // Test power operator
    result = execute_expression("2 ** 3");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(8.0, result.as.float64);
    vm_release(result);

    result = execute_expression("5 ** 0");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, result.as.float64);
    vm_release(result);

    result = execute_expression("4 ** 0.5");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, result.as.float64);
    vm_release(result);

    result = execute_expression("(-2) ** 3");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(-8.0, result.as.float64);
    vm_release(result);

    // Test power right associativity: 2 ** 3 ** 2 = 2 ** (3 ** 2) = 2 ** 9 = 512
    result = execute_expression("2 ** 3 ** 2");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(512.0, result.as.float64);
    vm_release(result);

    // Test power precedence: 2 * 3 ** 2 = 2 * (3 ** 2) = 2 * 9 = 18
    result = execute_expression("2 * 3 ** 2");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(18.0, result.as.float64);
    vm_release(result);

    // Test complex power precedence: 2 + 3 * 4 ** 2 = 2 + 3 * (4 ** 2) = 2 + 3 * 16 = 2 + 48 = 50
    result = execute_expression("2 + 3 * 4 ** 2");
    TEST_ASSERT_EQUAL_INT(VAL_FLOAT64, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(50.0, result.as.float64);
    vm_release(result);
}

// Test division by zero error handling (moved from test_vm.c)
void test_division_by_zero_errors() {
    value_t result;

    // Division by zero should return null (error case)
    result = execute_expression_allow_errors("10 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    result = execute_expression_allow_errors("0 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    // Negative division by zero
    result = execute_expression_allow_errors("-5 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);
}

// Test modulo by zero error handling (moved from test_vm.c)
void test_modulo_by_zero_errors() {
    value_t result;

    // Modulo by zero should return null (error case)
    result = execute_expression_allow_errors("10 mod 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    result = execute_expression_allow_errors("0 mod 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    // Negative modulo by zero
    result = execute_expression_allow_errors("-5 mod 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    // Float modulo by zero
    result = execute_expression_allow_errors("3.14 mod 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);
}

// Test comprehensive unary operations (moved from test_vm.c)
void test_comprehensive_unary() {
    value_t result;

    result = execute_expression("-42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-42, result.as.int32);
    vm_release(result);

    result = execute_expression("3 + -4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-1, result.as.int32);
    vm_release(result);

    result = execute_expression("-(-5)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test increment/decrement on array elements
void test_array_element_increment_decrement() {
    // Test pre-increment on array element
    {
        value_t result = execute_expression("var arr = [1, 2, 3]; ++arr(0)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
        vm_release(result);
    }
    
    // Test that array element is actually modified by pre-increment
    {
        value_t result = execute_expression("var arr = [5, 10, 15]; ++arr(1); arr(1)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(11, result.as.int32);
        vm_release(result);
    }
    
    // Test post-increment returns old value
    {
        value_t result = execute_expression("var arr = [20, 30, 40]; arr(2)++");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(40, result.as.int32);  // Returns old value
        vm_release(result);
    }
    
    // Test that array element is modified by post-increment
    {
        value_t result = execute_expression("var arr = [20, 30, 40]; arr(2)++; arr(2)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(41, result.as.int32);  // New value
        vm_release(result);
    }
    
    // Test pre-decrement on array element
    {
        value_t result = execute_expression("var arr = [10, 20, 30]; --arr(1)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(19, result.as.int32);
        vm_release(result);
    }
    
    // Test post-decrement returns old value
    {
        value_t result = execute_expression("var arr = [100, 200, 300]; arr(0)--");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(100, result.as.int32);  // Returns old value
        vm_release(result);
    }
    
    // Test that array element is modified by post-decrement
    {
        value_t result = execute_expression("var arr = [100, 200, 300]; arr(0)--; arr(0)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(99, result.as.int32);   // New value
        vm_release(result);
    }
}

// Test increment/decrement on object properties
void test_object_property_increment_decrement() {
    // Test pre-increment on object property
    {
        value_t result = execute_expression("var obj = {x: 5, y: 10}; ++obj.x");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(6, result.as.int32);
        vm_release(result);
    }
    
    // Test that object property is actually modified by pre-increment
    {
        value_t result = execute_expression("var obj = {a: 15, b: 25}; ++obj.b; obj.b");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(26, result.as.int32);
        vm_release(result);
    }
    
    // Test post-increment returns old value
    {
        value_t result = execute_expression("var obj = {count: 50}; obj.count++");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(50, result.as.int32);  // Returns old value
        vm_release(result);
    }
    
    // Test that object property is modified by post-increment
    {
        value_t result = execute_expression("var obj = {count: 50}; obj.count++; obj.count");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(51, result.as.int32);  // New value
        vm_release(result);
    }
    
    // Test pre-decrement on object property
    {
        value_t result = execute_expression("var obj = {value: 100}; --obj.value");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(99, result.as.int32);
        vm_release(result);
    }
    
    // Test post-decrement returns old value
    {
        value_t result = execute_expression("var obj = {score: 75}; obj.score--");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(75, result.as.int32);  // Returns old value
        vm_release(result);
    }
    
    // Test that object property is modified by post-decrement
    {
        value_t result = execute_expression("var obj = {score: 75}; obj.score--; obj.score");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(74, result.as.int32);  // New value
        vm_release(result);
    }
}

// Test comprehensive increment/decrement scenarios with arrays and objects
void test_increment_decrement_advanced_scenarios() {
    // Test mixed operations with arrays
    {
        value_t result = execute_expression("var arr = [1, 2, 3]; ++arr(0) + arr(1)++");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(4, result.as.int32);  // 2 + 2 (pre-inc returns 2, post-inc returns old 2)
        vm_release(result);
    }
    
    // Test that both modifications took effect
    {
        value_t result = execute_expression("var arr = [1, 2, 3]; ++arr(0) + arr(1)++; arr(0) + arr(1)");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(5, result.as.int32);   // 2 + 3 (both were incremented)
        vm_release(result);
    }
    
    // Test mixed operations with objects
    {
        value_t result = execute_expression("var obj = {a: 10, b: 20}; --obj.a + obj.b--");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(29, result.as.int32);  // 9 + 20 (pre-dec returns 9, post-dec returns old 20)
        vm_release(result);
    }
    
    // Test increment/decrement with overflow
    {
        value_t result = execute_expression("var arr = [2147483647]; ++arr(0)");
        TEST_ASSERT_EQUAL(VAL_BIGINT, result.type);
        char* str = di_to_string(result.as.bigint, 10);
        TEST_ASSERT_EQUAL_STRING("2147483648", str);
        free(str);
        vm_release(result);
    }
    
    // Test reference semantics - modifying nested object property
    {
        value_t result = execute_expression("var outer = {inner: {count: 5}}; ++outer.inner.count");
        TEST_ASSERT_EQUAL(VAL_INT32, result.type);
        TEST_ASSERT_EQUAL_INT32(6, result.as.int32);
        vm_release(result);
    }
}

// Test suite function for integration with main test runner
void test_arithmetic_suite(void) {
    RUN_TEST(test_basic_int32_arithmetic);
    RUN_TEST(test_int32_division_always_float);
    RUN_TEST(test_int32_overflow_promotion);
    RUN_TEST(test_mixed_int_float_arithmetic);
    RUN_TEST(test_operator_precedence_with_integers);
    RUN_TEST(test_unary_arithmetic);
    RUN_TEST(test_large_arithmetic);
    RUN_TEST(test_floor_division);
    RUN_TEST(test_increment_decrement);
    RUN_TEST(test_increment_decrement_comprehensive);
    RUN_TEST(test_invalid_increment_decrement_errors);
    RUN_TEST(test_array_element_increment_decrement);
    RUN_TEST(test_object_property_increment_decrement);
    RUN_TEST(test_increment_decrement_advanced_scenarios);
    RUN_TEST(test_comprehensive_arithmetic);
    RUN_TEST(test_modulo_operations);
    RUN_TEST(test_power_operations);
    RUN_TEST(test_comprehensive_unary);
    RUN_TEST(test_division_by_zero_errors);
    RUN_TEST(test_modulo_by_zero_errors);
}
