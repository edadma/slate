#include <math.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to compile and run code
value_t run_code(const char* source)
{
    lexer_t lexer;
    parser_t parser;

    lexer_init(&lexer, source);
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program)
    {
        return make_null();
    }

    codegen_t* codegen = codegen_create();
    function_t* function = codegen_compile(codegen, program);

    bitty_vm* vm = vm_create();
    vm_result result = vm_execute(vm, function);

    value_t return_value = make_null();
    if (result == VM_OK)
    {
        return_value = vm->result;
        // Retain strings and other reference-counted types to survive cleanup
        return_value = vm_retain(return_value);
    }

    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);

    return return_value;
}

// Test numeric operations
void test_vm_arithmetic(void)
{
    value_t result;

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
void test_vm_unary(void)
{
    value_t result;

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
void test_vm_strings(void)
{
    value_t result;

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
void test_vm_booleans(void)
{
    value_t result;

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
void test_vm_null(void)
{
    value_t result;

    result = run_code("null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    result = run_code("null + \" value\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null value", result.as.string);
    ds_release(&result.as.string);
}

// Test value creation functions
void test_vm_value_creation(void)
{
    value_t val;

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
void test_vm_value_equality(void)
{
    value_t a, b;

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
void test_vm_is_falsy(void)
{
    TEST_ASSERT_TRUE(is_falsy(make_null()));
    TEST_ASSERT_TRUE(is_falsy(make_boolean(0)));
    TEST_ASSERT_FALSE(is_falsy(make_boolean(1)));
    TEST_ASSERT_TRUE(is_falsy(make_number(0))); // 0 is falsy
    TEST_ASSERT_FALSE(is_falsy(make_number(42)));

    value_t str = make_string("");
    TEST_ASSERT_TRUE(is_falsy(str)); // empty string is falsy
    free_value(str);
}

// Test division by zero handling
void test_vm_division_by_zero(void)
{
    value_t result;

    // Division by zero should return null (error case)
    result = run_code("10 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    result = run_code("0 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Negative division by zero
    result = run_code("-5 / 0");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

// Test advanced string concatenation edge cases
void test_vm_string_concatenation_edge_cases(void)
{
    value_t result;

    // Null concatenation
    result = run_code("null + \" value\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null value", result.as.string);
    ds_release(&result.as.string);

    // Boolean + string concatenation (boolean will be converted to string)
    result = run_code("true + \" and false\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("true and false", result.as.string);
    ds_release(&result.as.string);

    // Empty string concatenation
    result = run_code("\"\" + \"\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    ds_release(&result.as.string);

    // Number to string conversion
    result = run_code("3.14159 + \" is pi\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    // Should use %.6g formatting
    ds_release(&result.as.string);

    // Boolean + string with number concatenation
    result = run_code("false + \"42\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("false42", result.as.string);
    ds_release(&result.as.string);
}

// Test array edge cases
void test_vm_arrays_edge_cases(void)
{
    value_t result;

    // Empty array
    result = run_code("[]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(0, da_length(result.as.array));
    free_value(result);

    // Empty array length
    result = run_code("[].length");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);

    // Out of bounds access should be error (returns null on error)
    result = run_code("[1, 2, 3](10)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case

    // Negative index should be error (returns null on error)
    result = run_code("[1, 2, 3](-1)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case

    // Mixed type array
    result = run_code("[1, \"hello\", true, null]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(4, da_length(result.as.array));
    free_value(result);

    // Nested array access
    result = run_code("[[1, 2], [3, 4]](0)(1)");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(2.0, result.as.number);
}

// Test string indexing edge cases
void test_vm_string_indexing_edge_cases(void)
{
    value_t result;

    // Empty string indexing should be error (returns null on error)
    result = run_code("\"\"(0)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case

    // Out of bounds string access should be error (returns null on error)
    result = run_code("\"hello\"(10)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case

    // Negative string index should be error (returns null on error)
    result = run_code("\"hello\"(-1)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case

    // Single character strings
    result = run_code("\"a\"(0)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("a", result.as.string);
    ds_release(&result.as.string);

    // Last character access
    result = run_code("\"hello\"(4)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("o", result.as.string);
    ds_release(&result.as.string);
}

// Test type error handling
void test_vm_type_errors(void)
{
    value_t result;

    // These should return null (error handled gracefully)
    // Cannot subtract strings
    result = run_code("\"hello\" - \"world\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot multiply strings
    result = run_code("\"hello\" * 5");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot divide booleans
    result = run_code("true / false");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot add booleans (no implicit string conversion without string operand)
    result = run_code("true + false");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot add boolean + number (no implicit string conversion without string operand)
    result = run_code("false + 42");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot negate strings
    result = run_code("-\"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot index numbers
    result = run_code("42(0)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Cannot index with non-numbers
    result = run_code("[1,2,3](\"hello\")");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

// Test complex expressions
void test_vm_complex_expressions(void)
{
    value_t result;

    // Array operations in expressions
    result = run_code("[1, 2, 3].length + 5");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(8.0, result.as.number);

    // String operations in expressions
    result = run_code("\"hello\".length * 2");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(10.0, result.as.number);

    // Array indexing in expressions
    result = run_code("[10, 20, 30](1) + 5");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(25.0, result.as.number);

    // Complex nested operations
    result = run_code("([1, 2, 3].length * 2) + [4, 5](1)");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(11.0, result.as.number); // (3 * 2) + 5 = 11
}

// Test property access edge cases
void test_vm_property_access_edge_cases(void)
{
    value_t result;

    // Invalid properties should return undefined
    result = run_code("[1, 2, 3].foo");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("\"hello\".foo");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("42.length");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    // Empty string length
    result = run_code("\"\".length");
    TEST_ASSERT_EQUAL_INT(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);

    // Null property access
    result = run_code("null.length");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
}

// Test undefined value behavior
void test_vm_undefined_behavior(void)
{
    value_t result;

    // Undefined literal
    result = run_code("undefined");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    // Undefined is falsy
    TEST_ASSERT_TRUE(is_falsy(make_undefined()));

    // Undefined equality
    TEST_ASSERT_TRUE(values_equal(make_undefined(), make_undefined()));
    TEST_ASSERT_FALSE(values_equal(make_undefined(), make_null()));
    TEST_ASSERT_FALSE(values_equal(make_undefined(), make_boolean(0)));
    TEST_ASSERT_FALSE(values_equal(make_undefined(), make_number(0)));

    // Property access returns undefined
    result = run_code("[1, 2, 3].nonExistent");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("\"hello\".nonExistent");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("42.anyProperty");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
}

// Test undefined string concatenation
void test_vm_undefined_string_concatenation(void)
{
    value_t result;

    // Undefined + string
    result = run_code("undefined + \" value\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("undefined value", result.as.string);
    ds_release(&result.as.string);

    // String + undefined
    result = run_code("\"value: \" + undefined");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("value: undefined", result.as.string);
    ds_release(&result.as.string);

    // Undefined + undefined (should fail - no string operand)
    result = run_code("undefined + undefined");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Error case
}

// Test suite runner
void test_vm_suite(void)
{
    RUN_TEST(test_vm_arithmetic);
    RUN_TEST(test_vm_unary);
    RUN_TEST(test_vm_strings);
    RUN_TEST(test_vm_booleans);
    RUN_TEST(test_vm_null);
    RUN_TEST(test_vm_value_creation);
    RUN_TEST(test_vm_value_equality);
    RUN_TEST(test_vm_is_falsy);
    RUN_TEST(test_vm_division_by_zero);
    RUN_TEST(test_vm_string_concatenation_edge_cases);
    RUN_TEST(test_vm_arrays_edge_cases);
    RUN_TEST(test_vm_string_indexing_edge_cases);
    RUN_TEST(test_vm_type_errors);
    RUN_TEST(test_vm_complex_expressions);
    RUN_TEST(test_vm_property_access_edge_cases);
    RUN_TEST(test_vm_undefined_behavior);
    RUN_TEST(test_vm_undefined_string_concatenation);
}
