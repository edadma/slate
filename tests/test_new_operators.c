#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "vm.h"
#include "ast.h"

// Test helper - based on existing run_code function
static value_t run_expression(const char* source) {
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
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    vm_result result = vm_execute(vm, function);

    value_t return_value = make_null();
    if (result == VM_OK) {
        return_value = vm->result;
        // Retain strings and other reference-counted types to survive cleanup
        return_value = vm_retain(return_value);
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);

    return return_value;
}

// Helper to run test and cleanup
static void test_expression_equals_int(const char* source, int expected) {
    value_t result = run_expression(source);
    assert(result.type == VAL_INT32);
    assert(result.as.int32 == expected);
    vm_release(result);
}

static void test_expression_equals_bool(const char* source, int expected) {
    value_t result = run_expression(source);
    assert(result.type == VAL_BOOLEAN);
    assert(result.as.boolean == expected);
    vm_release(result);
}

// Ternary operator tests
void test_ternary_true() {
    printf("Testing ternary operator with true condition...\n");
    test_expression_equals_int("true ? 42 : 100", 42);
    printf("✓ Ternary true test passed\n");
}

void test_ternary_false() {
    printf("Testing ternary operator with false condition...\n");
    test_expression_equals_int("false ? 42 : 100", 100);
    printf("✓ Ternary false test passed\n");
}

void test_ternary_nested() {
    printf("Testing nested ternary operators...\n");
    test_expression_equals_int("true ? (false ? 1 : 2) : 3", 2);
    printf("✓ Nested ternary test passed\n");
}

// Null coalescing operator tests
void test_null_coalesce_null() {
    printf("Testing null coalescing with null...\n");
    test_expression_equals_int("null " "?" "?" " 42", 42);
    printf("✓ Null coalesce with null test passed\n");
}

void test_null_coalesce_undefined() {
    printf("Testing null coalescing with undefined...\n");
    test_expression_equals_int("undefined " "?" "?" " 42", 42);
    printf("✓ Null coalesce with undefined test passed\n");
}

void test_null_coalesce_value() {
    printf("Testing null coalescing with value...\n");
    test_expression_equals_int("100 " "?" "?" " 42", 100);
    printf("✓ Null coalesce with value test passed\n");
}

void test_null_coalesce_chain() {
    printf("Testing null coalescing chain...\n");
    test_expression_equals_int("null " "?" "?" " undefined " "?" "?" " 42", 42);
    printf("✓ Null coalesce chain test passed\n");
}

// Null coalescing assignment tests
void test_null_coalesce_assign_null() {
    printf("Testing null coalescing assignment with null...\n");
    test_expression_equals_int("var x = null; x " "?" "?" "= 42; x", 42);
    printf("✓ Null coalesce assign with null test passed\n");
}

void test_null_coalesce_assign_value() {
    printf("Testing null coalescing assignment with value...\n");
    test_expression_equals_int("var x = 100; x " "?" "?" "= 42; x", 100);
    printf("✓ Null coalesce assign with value test passed\n");
}

// Shift assignment tests
void test_left_shift_assign() {
    printf("Testing left shift assignment...\n");
    test_expression_equals_int("var x = 5; x <<= 2; x", 20);  // 5 << 2 = 20
    printf("✓ Left shift assign test passed\n");
}

void test_right_shift_assign() {
    printf("Testing right shift assignment...\n");
    test_expression_equals_int("var x = 20; x >>= 2; x", 5);  // 20 >> 2 = 5
    printf("✓ Right shift assign test passed\n");
}

void test_logical_right_shift_assign() {
    printf("Testing logical right shift assignment...\n");
    test_expression_equals_int("var x = -20; x >>>= 28; x", 15);  // -20 >>> 28 = 15 (logical shift)
    printf("✓ Logical right shift assign test passed\n");
}

void test_shift_assign_zero() {
    printf("Testing shift assignment with zero...\n");
    test_expression_equals_int("var x = 0; x <<= 5; x", 0);  // 0 << 5 = 0
    printf("✓ Shift assign zero test passed\n");
}

// Property existence operator tests
void test_in_operator_exists() {
    printf("Testing 'in' operator with existing property...\n");
    test_expression_equals_bool("var obj = {a: 1, b: 2}; \"a\" in obj", 1);
    printf("✓ 'in' operator exists test passed\n");
}

void test_in_operator_not_exists() {
    printf("Testing 'in' operator with non-existing property...\n");
    test_expression_equals_bool("var obj = {a: 1, b: 2}; \"c\" in obj", 0);
    printf("✓ 'in' operator not exists test passed\n");
}

void test_in_operator_empty_object() {
    printf("Testing 'in' operator with empty object...\n");
    test_expression_equals_bool("var obj = {}; \"x\" in obj", 0);
    printf("✓ 'in' operator empty object test passed\n");
}

// instanceof operator tests
void test_instanceof_string_class() {
    printf("Testing 'instanceof' operator with String class...\n");
    test_expression_equals_bool("\"hello\" instanceof String", 1);
    printf("✓ String instanceof test passed\n");
}

void test_instanceof_localdate_class() {
    printf("Testing 'instanceof' operator with LocalDate class...\n");
    test_expression_equals_bool("LocalDate(2024, 12, 25) instanceof LocalDate", 1);
    printf("✓ LocalDate instanceof test passed\n");
}

void test_instanceof_array_class() {
    printf("Testing 'instanceof' operator with Array class...\n");
    test_expression_equals_bool("[1, 2, 3] instanceof Array", 1);
    printf("✓ Array instanceof test passed\n");
}

void test_instanceof_stringbuilder_class() {
    printf("Testing 'instanceof' operator with StringBuilder class...\n");
    test_expression_equals_bool("StringBuilder() instanceof StringBuilder", 1);
    printf("✓ StringBuilder instanceof test passed\n");
}

void test_instanceof_range_class() {
    printf("Testing 'instanceof' operator with Range class...\n");
    test_expression_equals_bool("(1..10) instanceof Range", 1);
    printf("✓ Range instanceof test passed\n");
}

void test_instanceof_negative_cases() {
    printf("Testing 'instanceof' operator negative cases...\n");
    test_expression_equals_bool("\"hello\" instanceof Array", 0);
    test_expression_equals_bool("[1, 2, 3] instanceof String", 0);
    test_expression_equals_bool("LocalDate(2024, 12, 25) instanceof StringBuilder", 0);
    printf("✓ instanceof negative cases test passed\n");
}

void test_instanceof_rejects_primitive_type_names() {
    printf("Testing 'instanceof' operator rejects primitive type names...\n");
    // This should cause a runtime error
    value_t result = run_expression("42 instanceof \"number\"");
    assert(result.type == VAL_NULL); // Runtime error results in null
    vm_release(result);
    printf("✓ instanceof correctly rejects primitive type names\n");
}

// Complex expression tests
void test_combined_operators() {
    printf("Testing combined operators...\n");
    test_expression_equals_int("var x = null; var y = x " "?" "?" " 5; var z = y > 3 ? 100 : 200; z", 100);
    printf("✓ Combined operators test passed\n");
}

void test_precedence() {
    printf("Testing operator precedence...\n");
    test_expression_equals_int("false " "?" "?" " true ? 1 : 2", 2);  // false ?? (true ? 1 : 2)
    printf("✓ Precedence test passed\n");
}

void run_new_operators_tests() {
    printf("\n=== New Operators Test Suite ===\n\n");
    
    // Ternary operator tests
    test_ternary_true();
    test_ternary_false();
    test_ternary_nested();
    
    // Null coalescing tests
    test_null_coalesce_null();
    test_null_coalesce_undefined();
    test_null_coalesce_value();
    test_null_coalesce_chain();
    
    // Null coalescing assignment tests
    test_null_coalesce_assign_null();
    test_null_coalesce_assign_value();
    
    // Shift assignment tests
    test_left_shift_assign();
    test_right_shift_assign();
    test_logical_right_shift_assign();
    test_shift_assign_zero();
    
    // Property existence tests
    test_in_operator_exists();
    test_in_operator_not_exists();
    test_in_operator_empty_object();
    
    // instanceof operator tests
    test_instanceof_string_class();
    test_instanceof_localdate_class();
    test_instanceof_array_class();
    test_instanceof_stringbuilder_class();
    test_instanceof_range_class();
    test_instanceof_negative_cases();
    test_instanceof_rejects_primitive_type_names();
    
    // Complex expression tests
    test_combined_operators();
    test_precedence();
    
    printf("\n=== All New Operator Tests Passed! ===\n\n");
}