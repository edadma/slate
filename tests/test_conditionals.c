#include <math.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "unity.h"
#include "vm.h"

// Helper function to compile and slate code
static value_t run_conditional_test(const char* source) {
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

// Test basic single-line if/then syntax
void test_single_line_if_then(void) {
    value_t result;

    // Simple if/then
    result = run_conditional_test("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = run_conditional_test("if false then 42");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // if/then/else
    result = run_conditional_test("if true then 42 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = run_conditional_test("if false then 42 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
}

// Test if/then with conditions
void test_if_then_with_conditions(void) {
    value_t result;

    // Comparison conditions
    result = run_conditional_test("if 5 > 3 then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    result = run_conditional_test("if 2 == 2 then 100 else 200");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(100, result.as.int32);

    // Logical conditions with symbolic operators
    result = run_conditional_test("if true && false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if true || false then 3 else 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);

    // Logical conditions with keyword operators
    result = run_conditional_test("if true and false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if true or false then 3 else 4");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);

    // Test 'not' keyword
    result = run_conditional_test("if not false then 5 else 6");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
}

// Test multi-line if/then with indented blocks
void test_multiline_if_then_blocks(void) {
    value_t result;

    // if/then with indented block
    result = run_conditional_test("if true then\n"
                                  "    var x = 10\n"
                                  "    x * 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // if without then, with indented block
    result = run_conditional_test("if true\n"
                                  "    var y = 5\n"
                                  "    y + 10");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);

    // if/then/else with indented blocks
    result = run_conditional_test("if false then\n"
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
    result = run_conditional_test("if false then 100\n"
                                  "else\n"
                                  "    var x = 20\n"
                                  "    x + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);

    // Multi-line then, single-line else
    result = run_conditional_test("if true then\n"
                                  "    var y = 30\n"
                                  "    y - 10\n"
                                  "else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // then on same line, block follows
    result = run_conditional_test("if true then\n"
                                  "    42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test nested if expressions
void test_nested_if_expressions(void) {
    value_t result;

    // Nested single-line
    result = run_conditional_test("if true then if false then 1 else 2 else 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // Nested multi-line
    result = run_conditional_test("if true\n"
                                  "    if false\n"
                                  "        100\n"
                                  "    else\n"
                                  "        200");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(200, result.as.int32);

    // Complex nesting with mixed forms
    result = run_conditional_test("if true then\n"
                                  "    var x = if false then 10 else 20\n"
                                  "    x + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);
}

// Test optional end markers
void test_end_markers(void) {
    value_t result;

    // Simple if with end if
    result = run_conditional_test("if true\n"
                                  "    42\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // if/else with end if
    result = run_conditional_test("if false\n"
                                  "    100\n"
                                  "else\n"
                                  "    200\n"
                                  "end if");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(200, result.as.int32);

    // Nested with end markers
    result = run_conditional_test("if true\n"
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
    result = run_conditional_test("var x = if true then 10 else 20\nx");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);

    // Use if in arithmetic
    result = run_conditional_test("5 + if false then 3 else 7");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32);

    // Use if in string concatenation
    result = run_conditional_test("\"Result: \" + if true then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Result: yes", result.as.string);
    vm_release(result);
}

// Test falsy/truthy values in conditions
void test_falsy_truthy_conditions(void) {
    value_t result;

    // Falsy values
    result = run_conditional_test("if false then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if null then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if undefined then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if 0 then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    result = run_conditional_test("if \"\" then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // Truthy values
    result = run_conditional_test("if true then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if 42 then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if \"hello\" then 1 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if [] then 1 else 2"); // Empty array is truthy
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);
}

// Test complex expressions with blocks
void test_complex_block_expressions(void) {
    value_t result;

    // Block with multiple statements and complex last expression
    result = run_conditional_test("if true\n"
                                  "    var base = 10\n"
                                  "    var multiplier = 3\n"
                                  "    base * multiplier + if false then 5 else 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(32, result.as.int32);

    // Nested blocks with variables
    result = run_conditional_test("if true\n"
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
    result = run_conditional_test("if true\n"
                                  "    42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Direct if-else without then
    result = run_conditional_test("if false\n"
                                  "    10\n"
                                  "else\n"
                                  "    20");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(20, result.as.int32);

    // Mixed: direct if with then else
    result = run_conditional_test("if true\n"
                                  "    5 + 5\n"
                                  "else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);
}

// Test comments in various positions
void test_comments(void) {
    value_t result;

    // Line comments
    result = run_conditional_test("\\ This is a comment\n"
                                  "if true then 42 \\ inline comment");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Block comments
    result = run_conditional_test("/* This is a\n"
                                  "   multi-line comment */\n"
                                  "if /* comment */ true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Comments in indented blocks
    result = run_conditional_test("if true\n"
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

    // Empty if block (should be null)
    result = run_conditional_test("if true\n"
                                  "    \\ Empty block\n");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // Multiple nested empty blocks
    result = run_conditional_test("if true\n"
                                  "    if false then null\n"
                                  "    else\n"
                                  "        42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Very deep nesting
    result = run_conditional_test("if true\n"
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
    result = run_conditional_test("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // 2. if condition then expression else expression
    result = run_conditional_test("if false then 42 else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(99, result.as.int32);

    // === CONDITION TYPES ===

    // Boolean literals
    result = run_conditional_test("if true then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if false then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // Numbers (truthy/falsy)
    result = run_conditional_test("if 1 then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if 0 then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // Strings (truthy/falsy)
    result = run_conditional_test("if \"hello\" then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    result = run_conditional_test("if \"\" then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // null and undefined
    result = run_conditional_test("if null then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    result = run_conditional_test("if undefined then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    // === COMPARISON OPERATORS ===

    // Equality
    result = run_conditional_test("if 5 == 5 then \"equal\" else \"not equal\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("equal", result.as.string);
    vm_release(result);

    result = run_conditional_test("if 5 != 3 then \"not equal\" else \"equal\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("not equal", result.as.string);
    vm_release(result);

    // Relational
    result = run_conditional_test("if 5 > 3 then \"greater\" else \"not greater\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("greater", result.as.string);
    vm_release(result);

    result = run_conditional_test("if 3 < 5 then \"less\" else \"not less\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("less", result.as.string);
    vm_release(result);

    result = run_conditional_test("if 5 >= 5 then \"gte\" else \"not gte\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("gte", result.as.string);
    vm_release(result);

    result = run_conditional_test("if 3 <= 5 then \"lte\" else \"not lte\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("lte", result.as.string);
    vm_release(result);

    // === EXPRESSION TYPES IN THEN/ELSE ===

    // Numbers
    result = run_conditional_test("if true then 42 else 99");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    // Strings
    result = run_conditional_test("if true then \"yes\" else \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    // Booleans
    result = run_conditional_test("if false then true else false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // null
    result = run_conditional_test("if false then 1 else null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === NESTED IF EXPRESSIONS ===

    // Simple nesting
    result = run_conditional_test("if true then if false then 1 else 2 else 3");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2, result.as.int32);

    // In condition
    result = run_conditional_test("if if true then true else false then 1 else 0");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(1, result.as.int32);

    // === ARITHMETIC IN CONDITIONS AND EXPRESSIONS ===

    // Arithmetic conditions
    result = run_conditional_test("if 2 + 3 == 5 then \"correct\" else \"wrong\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("correct", result.as.string);
    vm_release(result);

    // Arithmetic expressions
    result = run_conditional_test("if true then 2 * 3 else 4 + 5");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32);

    // === STRING OPERATIONS ===

    // String concatenation
    result = run_conditional_test("if true then \"Hello \" + \"World\" else \"Goodbye\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);

    // === IF WITHOUT ELSE (returns null when false) ===

    result = run_conditional_test("if true then 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = run_conditional_test("if false then 42");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

// Test logical operators comprehensively
void test_logical_operators(void) {
    value_t result;

    // === LOGICAL AND (&&, and) ===

    // Both symbolic and keyword forms with booleans
    result = run_conditional_test("true && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("true and false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = run_conditional_test("false && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = run_conditional_test("false and false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // AND with different value types (returns first falsy or last value)
    result = run_conditional_test("5 && \"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);

    result = run_conditional_test("0 and 42");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);

    result = run_conditional_test("null && \"never reached\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === LOGICAL OR (||, or) ===

    // Both symbolic and keyword forms with booleans
    result = run_conditional_test("true || false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("false or true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("true || true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("false or false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // OR with different value types (returns first truthy or last value)
    result = run_conditional_test("0 || \"fallback\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("fallback", result.as.string);
    vm_release(result);

    result = run_conditional_test("42 or \"never reached\"");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);

    result = run_conditional_test("\"\" || null");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);

    // === LOGICAL NOT (!, not) ===

    // Both symbolic and keyword forms
    result = run_conditional_test("!true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = run_conditional_test("not false");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    // NOT with different value types
    result = run_conditional_test("!42");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = run_conditional_test("not 0");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("!\"hello\"");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    result = run_conditional_test("not \"\"");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("!null");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("not undefined");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    // === COMPLEX COMBINATIONS ===

    // Mixed operators and precedence
    result = run_conditional_test("true and false or true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("not false && true");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);

    result = run_conditional_test("!(true or false)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);

    // With numbers and strings
    result = run_conditional_test("5 > 3 && \"yes\" || \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
    vm_release(result);

    result = run_conditional_test("0 or null or \"default\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("default", result.as.string);
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

    // TODO: Multiline indented block tests disabled due to lexer infinite loops
    RUN_TEST(test_multiline_if_then_blocks);
    RUN_TEST(test_mixed_single_multiline);
    RUN_TEST(test_nested_if_expressions);
    RUN_TEST(test_end_markers);
    RUN_TEST(test_complex_block_expressions);
    RUN_TEST(test_direct_if_blocks);
    RUN_TEST(test_edge_cases);
}
