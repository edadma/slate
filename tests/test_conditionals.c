#include <math.h>
#include <string.h>
#include "test_helpers.h"
#include "unity.h"


// Test basic single-line if/then syntax
void test_single_line_if_then(void) {
    value_t result;

    // Simple if/then
    result = test_execute_expression("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = test_execute_expression("if false then 42");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // if/then/else
    result = test_execute_expression("if true then 42 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = test_execute_expression("if false then 42 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
}

// Test if/then with conditions
void test_if_then_with_conditions(void) {
    value_t result;

    // Comparison conditions
    result = test_execute_expression("if 5 > 3 then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    result = test_execute_expression("if 2 == 2 then 100 else 200");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(100, result.as.int32);

    // Logical conditions with symbolic operators
    result = test_execute_expression("if true && false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if true || false then 3 else 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);

    // Logical conditions with keyword operators
    result = test_execute_expression("if true and false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if true or false then 3 else 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);

    // Test 'not' keyword
    result = test_execute_expression("if not false then 5 else 6");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
}

// Test multi-line if/then with indented blocks
void test_multiline_if_then_blocks(void) {
    value_t result;

    // if/then with indented block
    result = test_execute_expression("if true then\n"
                                  "    var x = 10\n"
                                  "    x * 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // if without then, with indented block
    result = test_execute_expression("if true\n"
                                  "    var y = 5\n"
                                  "    y + 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);

    // if/then/else with indented blocks
    result = test_execute_expression("if false then\n"
                                  "    100\n"
                                  "else\n"
                                  "    200");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(200, result.as.int32);
}

// Test mixed single-line and multi-line forms
void test_mixed_single_multiline(void) {
    value_t result;

    // Single-line then, multi-line else
    result = test_execute_expression("if false then 100\n"
                                  "else\n"
                                  "    var x = 20\n"
                                  "    x + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);

    // Multi-line then, single-line else
    result = test_execute_expression("if true then\n"
                                  "    var y = 30\n"
                                  "    y - 10\n"
                                  "else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // then on same line, block follows
    result = test_execute_expression("if true then\n"
                                  "    42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test nested if expressions
void test_nested_if_expressions(void) {
    value_t result;

    // Nested single-line
    result = test_execute_expression("if true then if false then 1 else 2 else 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // Nested multi-line
    result = test_execute_expression("if true\n"
                                  "    if false\n"
                                  "        100\n"
                                  "    else\n"
                                  "        200");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(200, result.as.int32);

    // Complex nesting with mixed forms
    result = test_execute_expression("if true then\n"
                                  "    var x = if false then 10 else 20\n"
                                  "    x + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);
}

// Test optional end markers
void test_end_markers(void) {
    value_t result;

    // Simple if with end if
    result = test_execute_expression("if true\n"
                                  "    42\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // if/else with end if
    result = test_execute_expression("if false\n"
                                  "    100\n"
                                  "else\n"
                                  "    200\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(200, result.as.int32);

    // Nested with end markers
    result = test_execute_expression("if true\n"
                                  "    if false\n"
                                  "        1\n"
                                  "    else\n"
                                  "        2\n"
                                  "    end if\n"
                                  "else\n"
                                  "    3\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
}

// Test if as expression (can be assigned to variables)
void test_if_as_expression(void) {
    value_t result;

    // Assign if result to variable
    result = test_execute_expression("var x = if true then 10 else 20\nx");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);

    // Use if in arithmetic
    result = test_execute_expression("5 + if false then 3 else 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32);

    // Use if in string concatenation
    result = test_execute_expression("\"Result: \" + if true then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Result: yes", result.as.string);
    vm_release(result);
}

// Test falsy/truthy values in conditions
void test_falsy_truthy_conditions(void) {
    value_t result;

    // Falsy values
    result = test_execute_expression("if false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if null then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if undefined then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if 0 then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = test_execute_expression("if \"\" then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // Truthy values
    result = test_execute_expression("if true then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if 42 then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if \"hello\" then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if [] then 1 else 2"); // Empty array is truthy
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
}

// Test complex expressions with blocks
void test_complex_block_expressions(void) {
    value_t result;

    // Block with multiple statements and complex last expression
    result = test_execute_expression("if true\n"
                                  "    var base = 10\n"
                                  "    var multiplier = 3\n"
                                  "    base * multiplier + if false then 5 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(32, result.as.int32);

    // Nested blocks with variables
    result = test_execute_expression("if true\n"
                                  "    var outer = 5\n"
                                  "    if true\n"
                                  "        var inner = outer * 2\n"
                                  "        inner + 3\n"
                                  "    else\n"
                                  "        0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(13, result.as.int32);
}


// Test direct if blocks (without 'then' keyword)
void test_direct_if_blocks(void) {
    value_t result;

    // Direct if without then
    result = test_execute_expression("if true\n"
                                  "    42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Direct if-else without then
    result = test_execute_expression("if false\n"
                                  "    10\n"
                                  "else\n"
                                  "    20");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // Mixed: direct if with then else
    result = test_execute_expression("if true\n"
                                  "    5 + 5\n"
                                  "else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);
}

// Test comments in various positions
void test_comments(void) {
    value_t result;

    // Line comments
    result = test_execute_expression("\\ This is a comment\n"
                                  "if true then 42 \\ inline comment");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Block comments
    result = test_execute_expression("/* This is a\n"
                                  "   multi-line comment */\n"
                                  "if /* comment */ true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Comments in indented blocks
    result = test_execute_expression("if true\n"
                                  "    \\ Comment in block\n"
                                  "    var x = 10\n"
                                  "    /* Another comment */\n"
                                  "    x * 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);
}

// Test edge cases and error conditions
void test_edge_cases(void) {
    value_t result;

    // If block with just null expression (should be null)
    result = test_execute_expression("if true\n"
                                  "    null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Multiple nested empty blocks
    result = test_execute_expression("if true\n"
                                  "    if false then null\n"
                                  "    else\n"
                                  "        42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Very deep nesting
    result = test_execute_expression("if true\n"
                                  "    if true\n"
                                  "        if true\n"
                                  "            if true\n"
                                  "                if true\n"
                                  "                    100\n"
                                  "                end if\n"
                                  "            end if\n"
                                  "        end if\n"
                                  "    end if\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(100, result.as.int32);
}

// Test comprehensive syntax variations that are actually implemented
void test_comprehensive_syntax_variations(void) {
    value_t result;

    // === BASIC IF FORMS ===

    // 1. if condition then expression
    result = test_execute_expression("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // 2. if condition then expression else expression
    result = test_execute_expression("if false then 42 else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(99, result.as.int32);

    // === CONDITION TYPES ===

    // Boolean literals
    result = test_execute_expression("if true then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if false then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // Numbers (truthy/falsy)
    result = test_execute_expression("if 1 then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if 0 then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // Strings (truthy/falsy)
    result = test_execute_expression("if \"hello\" then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = test_execute_expression("if \"\" then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // null and undefined
    result = test_execute_expression("if null then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    result = test_execute_expression("if undefined then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // === COMPARISON OPERATORS ===

    // Equality
    result = test_execute_expression("if 5 == 5 then \"equal\" else \"not equal\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("equal", result.as.string);
    vm_release(result);

    result = test_execute_expression("if 5 != 3 then \"not equal\" else \"equal\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("not equal", result.as.string);
    vm_release(result);

    // Relational
    result = test_execute_expression("if 5 > 3 then \"greater\" else \"not greater\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("greater", result.as.string);
    vm_release(result);

    result = test_execute_expression("if 3 < 5 then \"less\" else \"not less\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("less", result.as.string);
    vm_release(result);

    result = test_execute_expression("if 5 >= 5 then \"gte\" else \"not gte\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("gte", result.as.string);
    vm_release(result);

    result = test_execute_expression("if 3 <= 5 then \"lte\" else \"not lte\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("lte", result.as.string);
    vm_release(result);

    // === EXPRESSION TYPES IN THEN/ELSE ===

    // Numbers
    result = test_execute_expression("if true then 42 else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Strings
    result = test_execute_expression("if true then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    // Booleans
    result = test_execute_expression("if false then true else false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // null
    result = test_execute_expression("if false then 1 else null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === NESTED IF EXPRESSIONS ===

    // Simple nesting
    result = test_execute_expression("if true then if false then 1 else 2 else 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // In condition
    result = test_execute_expression("if if true then true else false then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    // === ARITHMETIC IN CONDITIONS AND EXPRESSIONS ===

    // Arithmetic conditions
    result = test_execute_expression("if 2 + 3 == 5 then \"correct\" else \"wrong\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("correct", result.as.string);
    vm_release(result);

    // Arithmetic expressions
    result = test_execute_expression("if true then 2 * 3 else 4 + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);

    // === STRING OPERATIONS ===

    // String concatenation
    result = test_execute_expression("if true then \"Hello \" + \"World\" else \"Goodbye\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);

    // === IF WITHOUT ELSE (returns null when false) ===

    result = test_execute_expression("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = test_execute_expression("if false then 42");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

// Test logical operators comprehensively
void test_logical_operators(void) {
    value_t result;

    // === LOGICAL AND (&&, and) ===

    // Both symbolic and keyword forms with booleans
    result = test_execute_expression("true && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("true and false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = test_execute_expression("false && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = test_execute_expression("false and false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // AND with different value types (returns first falsy or last value)
    result = test_execute_expression("5 && \"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);

    result = test_execute_expression("0 and 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    result = test_execute_expression("null && \"never reached\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === LOGICAL OR (||, or) ===

    // Both symbolic and keyword forms with booleans
    result = test_execute_expression("true || false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("false or true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("true || true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("false or false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // OR with different value types (returns first truthy or last value)
    result = test_execute_expression("0 || \"fallback\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("fallback", result.as.string);
    vm_release(result);

    result = test_execute_expression("42 or \"never reached\"");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = test_execute_expression("\"\" || null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === LOGICAL NOT (!, not) ===

    // Both symbolic and keyword forms
    result = test_execute_expression("!true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = test_execute_expression("not false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    // NOT with different value types
    result = test_execute_expression("!42");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = test_execute_expression("not 0");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("!\"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = test_execute_expression("not \"\"");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("!null");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("not undefined");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    // === COMPLEX COMBINATIONS ===

    // Mixed operators and precedence
    result = test_execute_expression("true and false or true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("not false && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = test_execute_expression("!(true or false)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // With numbers and strings
    result = test_execute_expression("5 > 3 && \"yes\" || \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    result = test_execute_expression("0 or null or \"default\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
    vm_release(result);
}

// Test basic elif functionality
void test_basic_elif(void) {
    value_t result;

    // Basic elif - first condition true
    result = test_execute_expression("var x = 10\n"
                                  "if x > 15 then \"huge\" elif x > 5 then \"big\" else \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("big", result.as.string);
    vm_release(result);

    // Basic elif - elif condition true
    result = test_execute_expression("var x = 3\n"
                                  "if x > 10 then \"huge\" elif x > 1 then \"medium\" else \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("medium", result.as.string);
    vm_release(result);

    // Basic elif - falls to else
    result = test_execute_expression("var x = 0\n"
                                  "if x > 10 then \"huge\" elif x > 1 then \"medium\" else \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("small", result.as.string);
    vm_release(result);

    // Single-line elif without else
    result = test_execute_expression("var x = 7\n"
                                  "if x > 10 then \"huge\" elif x > 5 then \"big\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("big", result.as.string);
    vm_release(result);
}

// Test multiple elif clauses
void test_multiple_elif(void) {
    value_t result;

    // Multiple elif clauses - first elif matches
    result = test_execute_expression("var score = 85\n"
                                  "if score >= 90 then \"A\"\n"
                                  "elif score >= 80 then \"B\"\n"
                                  "elif score >= 70 then \"C\"\n"
                                  "elif score >= 60 then \"D\"\n"
                                  "else \"F\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("B", result.as.string);
    vm_release(result);

    // Multiple elif clauses - middle elif matches
    result = test_execute_expression("var score = 75\n"
                                  "if score >= 90 then \"A\"\n"
                                  "elif score >= 80 then \"B\"\n"
                                  "elif score >= 70 then \"C\"\n"
                                  "elif score >= 60 then \"D\"\n"
                                  "else \"F\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("C", result.as.string);
    vm_release(result);

    // Multiple elif clauses - last elif matches
    result = test_execute_expression("var score = 65\n"
                                  "if score >= 90 then \"A\"\n"
                                  "elif score >= 80 then \"B\"\n"
                                  "elif score >= 70 then \"C\"\n"
                                  "elif score >= 60 then \"D\"\n"
                                  "else \"F\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("D", result.as.string);
    vm_release(result);

    // Multiple elif clauses - falls to else
    result = test_execute_expression("var score = 45\n"
                                  "if score >= 90 then \"A\"\n"
                                  "elif score >= 80 then \"B\"\n"
                                  "elif score >= 70 then \"C\"\n"
                                  "elif score >= 60 then \"D\"\n"
                                  "else \"F\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("F", result.as.string);
    vm_release(result);
}

// Test elif with different syntax variations
void test_elif_syntax_variations(void) {
    value_t result;

    // Mixed single-line and multi-line
    result = test_execute_expression("var x = 8\n"
                                  "if x > 10 then \"huge\"\n"
                                  "elif x > 5 then\n"
                                  "    \"big\"\n"
                                  "else \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("big", result.as.string);
    vm_release(result);

    // Elif without 'then' keyword (multi-line)
    result = test_execute_expression("var x = 3\n"
                                  "if x > 10\n"
                                  "    \"huge\"\n"
                                  "elif x > 1\n"
                                  "    \"medium\"\n"
                                  "else\n"
                                  "    \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("medium", result.as.string);
    vm_release(result);

    // All single-line
    result = test_execute_expression("var x = 12\n"
                                  "if x > 15 then \"huge\" elif x > 10 then \"big\" elif x > 5 then \"medium\" else \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("big", result.as.string);
    vm_release(result);
}

// Test elif with complex expressions and side effects
void test_elif_complex_expressions(void) {
    value_t result;

    // Elif with complex boolean expressions
    result = test_execute_expression("var x = 5\n"
                                  "var y = 3\n"
                                  "if x > 10 and y > 5 then \"both big\"\n"
                                  "elif x > 3 or y > 1 then \"at least one medium\"\n"
                                  "else \"both small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("at least one medium", result.as.string);
    vm_release(result);

    // Elif with arithmetic in conditions and bodies
    result = test_execute_expression("var num = 15\n"
                                  "if num mod 3 == 0 and num mod 5 == 0 then \"FizzBuzz\"\n"
                                  "elif num mod 3 == 0 then \"Fizz\"\n"
                                  "elif num mod 5 == 0 then \"Buzz\"\n"
                                  "else num");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("FizzBuzz", result.as.string);
    vm_release(result);

    // Elif with variable modification
    result = test_execute_expression("var count = 0\n"
                                  "var x = 7\n"
                                  "if x > 10 then count = count + 3\n"
                                  "elif x > 5 then count = count + 2\n"
                                  "elif x > 0 then count = count + 1\n"
                                  "count");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);
    vm_release(result);
}

// Test nested elif constructs
void test_nested_elif(void) {
    value_t result;

    // Nested if-elif inside elif
    result = test_execute_expression("var x = 5\n"
                                  "var y = 8\n"
                                  "if x > 10 then \"x big\"\n"
                                  "elif x > 3 then\n"
                                  "    if y > 10 then \"x medium, y big\"\n"
                                  "    elif y > 5 then \"x medium, y medium\"\n"
                                  "    else \"x medium, y small\"\n"
                                  "else \"x small\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("x medium, y medium", result.as.string);
    vm_release(result);

    // Elif chain with nested structure
    result = test_execute_expression("var category = \"B\"\n"
                                  "var level = 2\n"
                                  "if category == \"A\" then \"premium\"\n"
                                  "elif category == \"B\" then\n"
                                  "    if level > 5 then \"advanced\"\n"
                                  "    elif level > 2 then \"intermediate\"\n"
                                  "    else \"basic\"\n"
                                  "elif category == \"C\" then \"standard\"\n"
                                  "else \"unknown\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("basic", result.as.string);
    vm_release(result);
}

// Test elif edge cases
void test_elif_edge_cases(void) {
    value_t result;

    // Single elif without else
    result = test_execute_expression("var x = 3\n"
                                  "if x > 10 then \"big\" elif x > 1 then \"medium\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("medium", result.as.string);
    vm_release(result);

    // Multiple elif without else, none match
    result = test_execute_expression("var x = 0\n"
                                  "if x > 10 then \"big\" elif x > 5 then \"medium\" elif x > 1 then \"small\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
    vm_release(result);

    // Elif with null and undefined
    result = test_execute_expression("var x = null\n"
                                  "if x then \"truthy\" elif x == null then \"is null\" else \"other\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("is null", result.as.string);
    vm_release(result);

    // Elif with string comparisons
    result = test_execute_expression("var status = \"pending\"\n"
                                  "if status == \"complete\" then \"done\"\n"
                                  "elif status == \"in_progress\" then \"working\"\n"
                                  "elif status == \"pending\" then \"waiting\"\n"
                                  "else \"unknown\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("waiting", result.as.string);
    vm_release(result);
}

// Test return statements in if blocks (regression test for critical bug)
void test_return_in_if_blocks(void) {
    value_t result;
    
    // Test 1: Single return in if block with trailing return
    result = test_execute_expression(
        "def test1() =\n"
        "    if true then\n"
        "        return \"correct\"\n"
        "    return \"wrong\"\n"
        "test1()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("correct", result.as.string);
    vm_release(result);
    
    // Test 2: Single return in if block without else
    result = test_execute_expression(
        "def test2() =\n"
        "    if false then\n"
        "        return \"wrong\"\n"
        "    return \"correct\"\n"
        "test2()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("correct", result.as.string);
    vm_release(result);
    
    // Test 3: Return in both if and else blocks
    result = test_execute_expression(
        "def test3() =\n"
        "    if true then\n"
        "        return \"from_if\"\n"
        "    else\n"
        "        return \"from_else\"\n"
        "    return \"should_not_reach\"\n"
        "test3()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("from_if", result.as.string);
    vm_release(result);
    
    // Test 4: Return in else block
    result = test_execute_expression(
        "def test4() =\n"
        "    if false then\n"
        "        return \"from_if\"\n"
        "    else\n"
        "        return \"from_else\"\n"
        "    return \"should_not_reach\"\n"
        "test4()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("from_else", result.as.string);
    vm_release(result);
    
    // Test 5: Nested if with returns
    result = test_execute_expression(
        "def test5() =\n"
        "    if true then\n"
        "        if true then\n"
        "            return \"nested_correct\"\n"
        "        return \"outer_if\"\n"
        "    return \"function_end\"\n"
        "test5()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("nested_correct", result.as.string);
    vm_release(result);
    
    // Test 6: Return with other statements in block
    result = test_execute_expression(
        "def test6() =\n"
        "    val x = 0\n"
        "    if true then\n"
        "        val y = 1\n"
        "        return x + y\n"
        "    return 99\n"
        "test6()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
    
    // Test 7: Single-line if with return
    result = test_execute_expression(
        "def test7() = if true then return \"single_line_works\"\n"
        "test7()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("single_line_works", result.as.string);
    vm_release(result);
    
    // Test 8: Return with value expression
    result = test_execute_expression(
        "def test8() =\n"
        "    val base = 10\n"
        "    if true then\n"
        "        return base * 2 + 5\n"
        "    return 0\n"
        "test8()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);
    
    // Test 9: Return in elif block
    result = test_execute_expression(
        "def test9() =\n"
        "    if false then\n"
        "        return \"if\"\n"
        "    elif true then\n"
        "        return \"elif_correct\"\n"
        "    else\n"
        "        return \"else\"\n"
        "    return \"end\"\n"
        "test9()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("elif_correct", result.as.string);
    vm_release(result);
    
    // Test 10: Complex condition with return
    result = test_execute_expression(
        "def test10(x) =\n"
        "    if x > 0 && x < 10 then\n"
        "        return \"in_range\"\n"
        "    return \"out_of_range\"\n"
        "test10(5)"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("in_range", result.as.string);
    vm_release(result);
}

// Test suite runner
void test_conditionals_suite(void) {
    // Test all implemented single-line syntax variations
    RUN_TEST(test_single_line_if_then);
    RUN_TEST(test_if_then_with_conditions);
    RUN_TEST(test_if_as_expression);
    RUN_TEST(test_falsy_truthy_conditions);
    RUN_TEST(test_comments);
    RUN_TEST(test_comprehensive_syntax_variations);
    RUN_TEST(test_logical_operators);

    // Elif tests
    RUN_TEST(test_basic_elif);
    RUN_TEST(test_multiple_elif);
    RUN_TEST(test_elif_syntax_variations);
    RUN_TEST(test_elif_complex_expressions);
    RUN_TEST(test_nested_elif);
    RUN_TEST(test_elif_edge_cases);

    // TODO: Multiline indented block tests disabled due to lexer infinite loops
    RUN_TEST(test_multiline_if_then_blocks);
    RUN_TEST(test_mixed_single_multiline);
    RUN_TEST(test_nested_if_expressions);
    RUN_TEST(test_end_markers);
    RUN_TEST(test_complex_block_expressions);
    RUN_TEST(test_direct_if_blocks);
    RUN_TEST(test_edge_cases);
    
    // Regression test for return statements in if blocks bug
    RUN_TEST(test_return_in_if_blocks);
}
