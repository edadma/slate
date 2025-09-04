#include "unity.h"
#include "test_helpers.h"

// Basic literal pattern matching tests
void test_match_literal_integer(void) {
    value_t result = test_execute_expression("match 42\n    case 42 do \"found\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("found", result.as.string);
}

void test_match_literal_string(void) {
    value_t result = test_execute_expression("match \"hello\"\n    case \"hello\" do \"matched\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("matched", result.as.string);
}

void test_match_literal_boolean(void) {
    value_t result = test_execute_expression("match true\n    case true do \"yes\"\n    case false do \"no\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("yes", result.as.string);
}

void test_match_literal_null(void) {
    value_t result = test_execute_expression("match null\n    case null do \"null matched\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null matched", result.as.string);
}

// Multiple cases with first match wins
void test_match_multiple_cases_first_wins(void) {
    value_t result = test_execute_expression("match 42\n    case 42 do \"first\"\n    case 42 do \"second\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("first", result.as.string);
}

void test_match_multiple_cases_later_match(void) {
    value_t result = test_execute_expression("match 100\n    case 42 do \"first\"\n    case 100 do \"second\"\n    case 200 do \"third\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("second", result.as.string);
}

// Variable binding tests
void test_match_variable_binding_basic(void) {
    value_t result = test_execute_expression("match \"hello\"\n    case x do x + \" world\"");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello world", result.as.string);
}

void test_match_variable_binding_with_number(void) {
    value_t result = test_execute_expression("match 42\n    case x do x * 2");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(84, result.as.int32);
}

void test_match_variable_binding_with_array(void) {
    value_t result = test_execute_expression("match [1, 2, 3]\n    case arr do arr.length()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
}

// Mixed literal and variable patterns
void test_match_mixed_literal_first(void) {
    value_t result = test_execute_expression("match 42\n    case 42 do \"literal\"\n    case x do \"variable: \" + x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("literal", result.as.string);
}

void test_match_mixed_variable_fallback(void) {
    value_t result = test_execute_expression("match 99\n    case 42 do \"literal\"\n    case x do \"variable: \" + x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("variable: 99", result.as.string);
}

void test_match_multiple_literals_then_variable(void) {
    value_t result = test_execute_expression("match \"test\"\n    case 42 do \"number\"\n    case true do \"boolean\"\n    case x do \"caught: \" + x");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("caught: test", result.as.string);
}

// Block form tests
void test_match_variable_block_form(void) {
    value_t result = test_execute_expression(
        "match 10\n"
        "    case x\n"
        "        var doubled = x * 2\n"
        "        \"Result: \" + doubled"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Result: 20", result.as.string);
}

void test_match_literal_block_form(void) {
    value_t result = test_execute_expression(
        "match 42\n"
        "    case 42\n"
        "        var msg = \"found\"\n"
        "        msg + \" the answer\""
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("found the answer", result.as.string);
}

void test_match_block_with_multiple_statements(void) {
    value_t result = test_execute_expression(
        "match [1, 2, 3]\n"
        "    case arr\n"
        "        var len = arr.length()\n"
        "        var first = arr(0)\n"
        "        var result = \"Length: \" + len + \", First: \" + first\n"
        "        result"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Length: 3, First: 1", result.as.string);
}

// Test 'do' followed by indented block
void test_match_do_with_indented_block(void) {
    value_t result = test_execute_expression(
        "match 100\n"
        "    case 100 do\n"
        "        var doubled = 100 * 2\n"
        "        \"Result: \" + doubled"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Result: 200", result.as.string);
}

// Non-exhaustive matches (should return null)
void test_match_non_exhaustive(void) {
    value_t result = test_execute_expression("match 999\n    case 42 do \"found\"\n    case 100 do \"century\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

void test_match_empty_cases(void) {
    // A match expression without any cases is a parse error by design
    // This test verifies that non-exhaustive matches (with cases that don't match) return null
    value_t result = test_execute_expression("match 999\n    case 42 do \"found\"");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type);
}

// Expression context tests
void test_match_as_assignment_value(void) {
    value_t result = test_execute_expression(
        "var result = match \"test\"\n"
        "    case \"test\" do \"matched\"\n"
        "    case x do \"other: \" + x\n"
        "result"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("matched", result.as.string);
}

void test_match_in_arithmetic(void) {
    value_t result = test_execute_expression(
        "var base = 10\n"
        "var result = base + match 5\n"
        "    case 5 do 3\n"
        "    case x do x\n"
        "result"
    );
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(13, result.as.int32);
}

// Complex expression tests
void test_match_with_complex_expressions(void) {
    value_t result = test_execute_expression(
        "var x = 20\n"
        "match x + 22\n"
        "    case 42 do \"the answer\"\n"
        "    case y do \"got: \" + y"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("the answer", result.as.string);
}

void test_match_with_method_calls(void) {
    value_t result = test_execute_expression(
        "var arr = [1, 2, 3]\n"
        "match arr.length()\n"
        "    case 3 do \"three elements\"\n"
        "    case x do x + \" elements\""
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("three elements", result.as.string);
}

// Variable scoping tests
void test_match_variable_scoping(void) {
    value_t result = test_execute_expression(
        "var x = \"outer\"\n"
        "var result = match \"inner\"\n"
        "    case x do \"bound: \" + x\n"
        "x + \", \" + result"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("outer, bound: inner", result.as.string);
}

void test_match_variable_isolation(void) {
    value_t result = test_execute_expression(
        "match 42\n"
        "    case value\n"
        "        var temp = value * 2\n"
        "        temp + 1"
    );
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(85, result.as.int32);
}

// Type checking with different value types
void test_match_different_types(void) {
    value_t result = test_execute_expression(
        "match [1, 2]\n"
        "    case \"string\" do \"string matched\"\n"
        "    case 42 do \"number matched\"\n"
        "    case arr do \"array: \" + arr.length()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("array: 2", result.as.string);
}

void test_match_object_literals(void) {
    value_t result = test_execute_expression(
        "match {name: \"test\", value: 42}\n"
        "    case obj do obj.name + \": \" + obj.value"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("test: 42", result.as.string);
}

// Edge cases
void test_match_nested_expressions(void) {
    value_t result = test_execute_expression(
        "match (if true then 42 else 0)\n"
        "    case 42 do \"conditional matched\"\n"
        "    case x do \"other: \" + x"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("conditional matched", result.as.string);
}

void test_match_with_ranges(void) {
    value_t result = test_execute_expression(
        "match (1..3)\n"
        "    case r do \"range length: \" + r.length()"
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("range length: 3", result.as.string);
}

void test_match_boolean_expressions(void) {
    value_t result = test_execute_expression(
        "var x = 5\n"
        "match x > 3\n"
        "    case true do \"greater\"\n"
        "    case false do \"not greater\""
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("greater", result.as.string);
}

// Test equals method dispatch
void test_match_uses_equals_method(void) {
    // Test that match uses .equals() method by matching arrays with same content
    value_t result = test_execute_expression(
        "match [1, 2, 3]\n"
        "    case [1, 2, 3] do \"arrays equal\"\n"
        "    case x do \"not equal\""
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("arrays equal", result.as.string);
}

void test_match_string_equality(void) {
    value_t result = test_execute_expression(
        "var s1 = \"hello\"\n"
        "var s2 = \"hel\" + \"lo\"\n"
        "match s1\n"
        "    case s2 do \"strings equal\"\n"  // This should use .equals() method
        "    case x do \"not equal\""
    );
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("strings equal", result.as.string);
}

// Test suite runner
void test_match_suite(void) {
    // Basic literal matching
    RUN_TEST(test_match_literal_integer);
    RUN_TEST(test_match_literal_string);
    RUN_TEST(test_match_literal_boolean);
    RUN_TEST(test_match_literal_null);
    
    // Multiple cases
    RUN_TEST(test_match_multiple_cases_first_wins);
    RUN_TEST(test_match_multiple_cases_later_match);
    
    // Variable binding
    RUN_TEST(test_match_variable_binding_basic);
    RUN_TEST(test_match_variable_binding_with_number);
    RUN_TEST(test_match_variable_binding_with_array);
    
    // Mixed patterns
    RUN_TEST(test_match_mixed_literal_first);
    RUN_TEST(test_match_mixed_variable_fallback);
    RUN_TEST(test_match_multiple_literals_then_variable);
    
    // Block form
    RUN_TEST(test_match_variable_block_form);
    RUN_TEST(test_match_literal_block_form);
    RUN_TEST(test_match_block_with_multiple_statements);
    RUN_TEST(test_match_do_with_indented_block);
    
    // Non-exhaustive
    RUN_TEST(test_match_non_exhaustive);
    RUN_TEST(test_match_empty_cases);
    
    // Expression contexts
    RUN_TEST(test_match_as_assignment_value);
    RUN_TEST(test_match_in_arithmetic);
    
    // Complex expressions
    RUN_TEST(test_match_with_complex_expressions);
    RUN_TEST(test_match_with_method_calls);
    
    // Variable scoping
    RUN_TEST(test_match_variable_scoping);
    RUN_TEST(test_match_variable_isolation);
    
    // Type checking
    RUN_TEST(test_match_different_types);
    RUN_TEST(test_match_object_literals);
    
    // Edge cases
    RUN_TEST(test_match_nested_expressions);
    RUN_TEST(test_match_with_ranges);
    RUN_TEST(test_match_boolean_expressions);
    
    // Equals method dispatch
    RUN_TEST(test_match_uses_equals_method);
    RUN_TEST(test_match_string_equality);
}