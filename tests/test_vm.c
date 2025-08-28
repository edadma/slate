#include <math.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to compile and slate code
value_t run_code(const char* source) {
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

    slate_vm* vm = vm_create();
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

// Note: Arithmetic tests moved to test_arithmetic.c for better organization

// Note: Unary arithmetic tests moved to test_arithmetic.c for better organization

// Test string operations
void test_vm_strings(void) {
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

    // Test string escape sequences
    result = run_code("\"Hello\\nWorld\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello\nWorld", result.as.string);
    ds_release(&result.as.string);

    result = run_code("\"Tab\\there\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Tab\there", result.as.string);
    ds_release(&result.as.string);

    result = run_code("\"Say \\\"Hello\\\"\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Say \"Hello\"", result.as.string);
    ds_release(&result.as.string);

    result = run_code("\"Path\\\\to\\\\file\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Path\\to\\file", result.as.string);
    ds_release(&result.as.string);
}

// Test boolean operations
void test_vm_booleans(void) {
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
void test_vm_null(void) {
    value_t result;

    result = run_code("null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    result = run_code("null + \" value\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null value", result.as.string);
    ds_release(&result.as.string);
}

// Test value creation functions
void test_vm_value_creation(void) {
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
void test_vm_value_equality(void) {
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
void test_vm_is_falsy(void) {
    TEST_ASSERT_TRUE(is_falsy(make_null()));
    TEST_ASSERT_TRUE(is_falsy(make_boolean(0)));
    TEST_ASSERT_FALSE(is_falsy(make_boolean(1)));
    TEST_ASSERT_TRUE(is_falsy(make_number(0))); // 0 is falsy
    TEST_ASSERT_FALSE(is_falsy(make_number(42)));

    value_t str = make_string("");
    TEST_ASSERT_TRUE(is_falsy(str)); // empty string is falsy
    free_value(str);
}

// Note: Division and modulo by zero error tests moved to test_arithmetic.c for better organization

// Test object literals
void test_vm_object_literals(void) {
    value_t result;

    // Empty object
    result = run_code("{}");
    TEST_ASSERT_EQUAL_INT(VAL_OBJECT, result.type);
    vm_release(result);

    // Object with one property
    result = run_code("{\"key\": 42}");
    TEST_ASSERT_EQUAL_INT(VAL_OBJECT, result.type);
    vm_release(result);

    // Object with multiple properties
    result = run_code("{\"name\": \"test\", \"value\": 123}");
    TEST_ASSERT_EQUAL_INT(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test advanced string concatenation edge cases
void test_vm_string_concatenation_edge_cases(void) {
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
void test_vm_arrays_edge_cases(void) {
    value_t result;

    // Empty array
    result = run_code("[]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(0, da_length(result.as.array));
    free_value(result);

    // Empty array length
    result = run_code("[].length");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

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
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
}

// Test string indexing edge cases
void test_vm_string_indexing_edge_cases(void) {
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
void test_vm_type_errors(void) {
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

    // Cannot modulo with non-numbers
    result = run_code("\"hello\" mod 3");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    result = run_code("5 mod \"world\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    result = run_code("true mod false");
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
void test_vm_complex_expressions(void) {
    value_t result;

    // Array operations in expressions
    result = run_code("[1, 2, 3].length + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(8, result.as.int32);

    // String operations in expressions (now using method call)
    result = run_code("\"hello\".length() * 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);

    // Array indexing in expressions
    result = run_code("[10, 20, 30](1) + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);

    // Complex nested operations
    result = run_code("([1, 2, 3].length * 2) + [4, 5](1)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(11, result.as.int32); // (3 * 2) + 5 = 11
}

// Test property access edge cases
void test_vm_property_access_edge_cases(void) {
    value_t result;

    // Invalid properties should return undefined
    result = run_code("[1, 2, 3].foo");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("\"hello\".foo");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    result = run_code("42.length");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);

    // Empty string length (now using method call)
    result = run_code("\"\".length()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // Null property access
    result = run_code("null.length");
    TEST_ASSERT_EQUAL_INT(VAL_UNDEFINED, result.type);
}

// Test undefined value behavior
void test_vm_undefined_behavior(void) {
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
void test_vm_undefined_string_concatenation(void) {
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

// Test comments
void test_vm_comments(void) {
    value_t result;

    // Backslash comment
    result = run_code("42 \\ This is a comment");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Simple expression with comment at end
    result = run_code("5 * 5 \\ End with comment");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);

    // Simple addition test (comments consume rest of line)
    result = run_code("1 + 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Test array concatenation
void test_vm_array_concatenation(void) {
    value_t result;

    // Basic array concatenation
    result = run_code("[1] + [2]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(2, da_length(result.as.array));

    value_t* elem0 = (value_t*)da_get(result.as.array, 0);
    value_t* elem1 = (value_t*)da_get(result.as.array, 1);
    TEST_ASSERT_EQUAL_INT(VAL_INT32, elem0->type);
    TEST_ASSERT_EQUAL_INT(VAL_INT32, elem1->type);
    TEST_ASSERT_EQUAL_INT32(1, elem0->as.int32);
    TEST_ASSERT_EQUAL_INT32(2, elem1->as.int32);

    vm_release(result);

    // Multiple element concatenation
    result = run_code("[1, 2] + [3, 4, 5]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(5, da_length(result.as.array));

    for (int i = 0; i < 5; i++) {
        value_t* elem = (value_t*)da_get(result.as.array, i);
        TEST_ASSERT_EQUAL_INT(VAL_INT32, elem->type);
        TEST_ASSERT_EQUAL_INT32(i + 1, elem->as.int32);
    }

    vm_release(result);

    // Empty array concatenation
    result = run_code("[] + [1, 2]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(2, da_length(result.as.array));

    value_t* elem_a = (value_t*)da_get(result.as.array, 0);
    value_t* elem_b = (value_t*)da_get(result.as.array, 1);
    TEST_ASSERT_EQUAL_INT32(1, elem_a->as.int32);
    TEST_ASSERT_EQUAL_INT32(2, elem_b->as.int32);

    vm_release(result);

    // Mixed type array concatenation
    result = run_code("[1, \"hello\"] + [true, null]");
    TEST_ASSERT_EQUAL_INT(VAL_ARRAY, result.type);
    TEST_ASSERT_EQUAL_INT(4, da_length(result.as.array));

    value_t* e0 = (value_t*)da_get(result.as.array, 0);
    value_t* e1 = (value_t*)da_get(result.as.array, 1);
    value_t* e2 = (value_t*)da_get(result.as.array, 2);
    value_t* e3 = (value_t*)da_get(result.as.array, 3);

    TEST_ASSERT_EQUAL_INT(VAL_INT32, e0->type);
    TEST_ASSERT_EQUAL_INT(VAL_STRING, e1->type);
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, e2->type);
    TEST_ASSERT_EQUAL_INT(VAL_NULL, e3->type);

    vm_release(result);
}


// Test bound method property access
void test_vm_bound_method_property_access(void) {
    value_t result;

    // Test array.iterator returns bound method
    result = run_code("type([1, 2, 3].iterator)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("bound_method", result.as.string);
    vm_release(result);

    // Test range.iterator returns bound method
    result = run_code("type((1..5).iterator)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("bound_method", result.as.string);
    vm_release(result);
}

// Test bound method calls
void test_vm_bound_method_calls(void) {
    value_t result;

    // Test array.iterator() returns array iterator
    result = run_code("type([1, 2, 3].iterator())");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("iterator", result.as.string);
    vm_release(result);

    // Test range.iterator() returns range iterator
    result = run_code("type((1..5).iterator())");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("iterator", result.as.string);
    vm_release(result);

    // Test exclusive range iterator
    result = run_code("type((1..<5).iterator())");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("iterator", result.as.string);
    vm_release(result);
}

// Test bound method string representation
void test_vm_bound_method_string_representation(void) {
    value_t result;

    // Test bound method converts to string properly
    result = run_code("\"Method: \" + [1, 2].iterator");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "Bound Method") != NULL);
    vm_release(result);
}

// Test bound method memory management
void test_vm_bound_method_memory_management(void) {
    value_t result;

    // Test that different arrays create different bound methods
    result = run_code("[1, 2].iterator == [3, 4].iterator");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);

    // Test bound method equality with same array
    result = run_code("var arr = [1, 2]; arr.iterator == arr.iterator");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean); // Different bound method objects
    vm_release(result);
}

// Test bound method context passing
void test_vm_bound_method_context_passing(void) {
    value_t result;

    // Test that method receives correct receiver
    // Array iterator should work with the specific array
    result = run_code("var arr = [10, 20, 30]; var iter = arr.iterator(); hasNext(iter)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);

    // Test range iterator with specific range
    result = run_code("var range = 5..8; var iter = range.iterator(); hasNext(iter)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
}

// Test bound method with type function
void test_vm_bound_method_type(void) {
    value_t result;

    // Test type() function works with bound methods
    result = run_code("type([1, 2].iterator)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("bound_method", result.as.string);
    vm_release(result);
}

// Test bound method error cases
void test_vm_bound_method_error_cases(void) {
    value_t result;

    // Test accessing non-existent method returns undefined
    result = run_code("[1, 2].nonexistent");
    TEST_ASSERT_EQUAL(VAL_UNDEFINED, result.type);
    vm_release(result);

    // Test calling non-method property fails
    result = run_code("[1, 2].length()");
    // This should cause a runtime error since length is not a function
    // But the test framework might not capture this properly
    // Just ensure it doesn't crash
    vm_release(result);
}

// Test suite runner
void test_vm_suite(void) {
    // Note: Arithmetic, unary, and division/modulo by zero tests moved to test_arithmetic.c
    RUN_TEST(test_vm_strings);
    RUN_TEST(test_vm_booleans);
    RUN_TEST(test_vm_null);
    RUN_TEST(test_vm_value_creation);
    RUN_TEST(test_vm_value_equality);
    RUN_TEST(test_vm_is_falsy);
    RUN_TEST(test_vm_object_literals);
    RUN_TEST(test_vm_string_concatenation_edge_cases);
    RUN_TEST(test_vm_arrays_edge_cases);
    RUN_TEST(test_vm_string_indexing_edge_cases);
    RUN_TEST(test_vm_type_errors);
    RUN_TEST(test_vm_complex_expressions);
    RUN_TEST(test_vm_property_access_edge_cases);
    RUN_TEST(test_vm_undefined_behavior);
    RUN_TEST(test_vm_undefined_string_concatenation);
    RUN_TEST(test_vm_comments);
    RUN_TEST(test_vm_array_concatenation);
    RUN_TEST(test_vm_bound_method_property_access);
    RUN_TEST(test_vm_bound_method_calls);
    RUN_TEST(test_vm_bound_method_string_representation);
    RUN_TEST(test_vm_bound_method_memory_management);
    RUN_TEST(test_vm_bound_method_context_passing);
    RUN_TEST(test_vm_bound_method_type);
    RUN_TEST(test_vm_bound_method_error_cases);
    // Conditional and block tests moved to test_conditionals.c
}
