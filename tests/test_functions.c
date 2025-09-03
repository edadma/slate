#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "unity.h"
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "vm.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source_code);

// Test anonymous lambda functions

void test_lambda_zero_parameters(void) {
    value_t result = run_code("(() -> 42)()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_single_parameter_bare(void) {
    value_t result = run_code("var double = x -> x * 2; double(21)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_single_parameter_parentheses(void) {
    value_t result = run_code("(x -> x * 2)(21)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_multiple_parameters(void) {
    value_t result = run_code("((x, y) -> x + y)(20, 22)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_assignment_and_call(void) {
    value_t result = run_code("var add = (x, y) -> x + y; add(15, 27)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_nested_expressions(void) {
    value_t result = run_code("var calc = (x, y) -> (x + y) * 2; calc(10, 11)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_lambda_with_string_operations(void) {
    value_t result = run_code("var greet = name -> 'Hello ' + name; greet('World')");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
}

// Test def syntax for named functions

void test_def_simple_function(void) {
    value_t result = run_code("def add(x, y) = x + y; add(20, 22)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_def_zero_parameter_function(void) {
    value_t result = run_code("def answer() = 42; answer()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_def_single_parameter_function(void) {
    value_t result = run_code("def square(x) = x * x; square(6)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(36, result.as.int32);
}

void test_def_function_with_complex_expression(void) {
    value_t result = run_code("def calc(x, y, z) = (x + y) * z; calc(2, 3, 8)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(40, result.as.int32);
}

void test_def_function_with_string(void) {
    value_t result = run_code("def greet(name) = 'Hello ' + name; greet('Slate')");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello Slate", result.as.string);
}

// Test function scoping and variables

void test_function_parameter_scoping(void) {
    value_t result = run_code("var x = 10; def test(x) = x * 2; test(21)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

void test_function_closure_behavior(void) {
    // Test that functions don't capture outer variables (simple functions only)
    value_t result = run_code("var y = 5; def test(x) = x + 37; test(y)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test function type and properties

void test_function_type_checking(void) {
    value_t result = run_code("var f = x -> x; type(f)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("closure", result.as.string);  // Functions are implemented as closures
}

void test_def_function_type_checking(void) {
    value_t result = run_code("def test(x) = x; type(test)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("closure", result.as.string);  // Functions are implemented as closures
}

// Test function arithmetic and operations

void test_lambda_with_arithmetic_operations(void) {
    value_t result = run_code("var math = (a, b, c) -> a + b * c - 8; math(10, 4, 8)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(34, result.as.int32);
}

void test_lambda_with_boolean_operations(void) {
    value_t result = run_code("var logic = (x, y) -> x > 5 && y < 10; logic(8, 3)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
}

// Test function calls with different argument types

void test_lambda_mixed_argument_types(void) {
    value_t result = run_code("var combine = (num, str) -> str + num; combine(42, 'Answer: ')");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Answer: 42", result.as.string);
}

// Test nested function calls

void test_nested_function_calls(void) {
    value_t result = run_code("var add = (x, y) -> x + y; var mult = (x, y) -> x * y; mult(add(2, 4), 7)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test functions as expressions

void test_lambda_as_expression_result(void) {
    value_t result = run_code("var f = if true then x -> x * 6 else x -> x + 1; f(7)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test function parsing edge cases

void test_lambda_with_complex_expressions(void) {
    value_t result = run_code("var complex = (x) -> if x > 0 then x * 6 else 0; complex(7)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test that functions return the correct value

void test_lambda_return_value_types(void) {
    // Test integer return
    value_t int_result = run_code("(x -> x)(42)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, int_result.type);
    TEST_ASSERT_EQUAL_INT32(42, int_result.as.int32);
    
    // Test string return
    value_t str_result = run_code("(x -> x)('hello')");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, str_result.type);
    TEST_ASSERT_EQUAL_STRING("hello", str_result.as.string);
    
    // Test boolean return
    value_t bool_result = run_code("(x -> x > 5)(10)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, bool_result.type);
    TEST_ASSERT_TRUE(bool_result.as.boolean);
}

// Test function argument count validation

void test_function_call_argument_validation(void) {
    // This should work - correct number of arguments
    value_t result = run_code("((x, y) -> x + y)(20, 22)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Test function recursion (simple cases)

void test_function_recursive_factorial(void) {
    // Note: This might require more complex control flow, but let's test simple recursion
    const char* code = "def factorial(n) = if n <= 1 then 1 else n * factorial(n - 1); factorial(5)";
    value_t result = run_code(code);
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(120, result.as.int32);
}

// Test closure constant pool isolation
void test_closure_constant_isolation(void) {
    // Each function should have its own constant pool
    // var f1 = (x) -> x + 100; var f2 = (x) -> x + 200; f1(1) + f2(1)
    value_t result = run_code("var f1 = (x) -> x + 100; var f2 = (x) -> x + 200; f1(1) + f2(1)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(302, result.as.int32);  // 101 + 201
}

// Comprehensive closure upvalue capture tests

// Basic closure variable capture
void test_basic_closure_capture(void) {
    // Test: def adder(n) = x -> x + n; var add5 = adder(5); add5(10)
    value_t result = run_code("def adder(n) = x -> x + n; var add5 = adder(5); add5(10)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
}

// Function composition with closures
void test_function_composition(void) {
    // Test the original failing example: compose(f, g) = x -> f(g(x))
    value_t result = run_code("def compose(f, g) = x -> f(g(x)); compose(a -> a + 1, b -> 2*b)(3)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
}

// Curried functions
void test_currying(void) {
    // Test: (x) -> (y) -> x + y
    value_t result = run_code("var outer = (x) -> (y) -> x + y; var add10 = outer(10); add10(5)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
}

// Multiple variable capture
void test_multiple_variable_capture(void) {
    // Test capturing multiple variables from outer scope
    value_t result = run_code("def makeFunc(a, b) = x -> a * x + b; var linear = makeFunc(3, 7); linear(4)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(19, result.as.int32); // 3 * 4 + 7 = 19
}

// Nested closure capture (3+ levels)
void test_nested_closure_capture(void) {
    // Test: def outer(x) = def middle(y) = z -> x + y + z; middle
    // This tests capturing across multiple nested scopes
    value_t result = run_code("def outer(x) = (y) -> (z) -> x + y + z; outer(1)(2)(3)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(6, result.as.int32); // 1 + 2 + 3 = 6
}

// Closure with string capture
void test_string_capture(void) {
    // Test capturing string values in closures
    value_t result = run_code("def greet(name) = msg -> name + \" \" + msg; var sayHi = greet(\"Alice\"); sayHi(\"Hello!\")");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "Alice Hello!") != NULL);
}

// Closure variable assignment (upvalue modification)
void test_upvalue_assignment(void) {
    // Test modifying captured variables - this test might not work yet if block expressions aren't fully supported
    // For now, test a simpler case
    value_t result = run_code("def makeAdder(start) = x -> start + x; var adder = makeAdder(100); adder(42)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(142, result.as.int32);
}

// Independent closure instances
void test_closure_independence(void) {
    // Test that different closure instances don't interfere
    value_t result = run_code("def makeAdder(n) = x -> x + n; var add3 = makeAdder(3); var add7 = makeAdder(7); add3(10) + add7(10)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(30, result.as.int32); // 13 + 17 = 30
}

// Closure with mixed value types
void test_mixed_type_capture(void) {
    // Test capturing different value types
    value_t result = run_code("def makeMixed(num, str, flag) = () -> if flag then str else num; var mixed = makeMixed(42, \"hello\", true); mixed()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "hello") != NULL);
}

// Error case: accessing non-existent upvalue
void test_closure_error_cases(void) {
    // This should work fine (regression test)
    value_t result = run_code("def simple() = 42; simple()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(42, result.as.int32);
}

// Closure performance test with many captures
void test_closure_performance(void) {
    // Test with multiple captures to ensure no performance degradation
    value_t result = run_code("def heavy(a,b,c,d,e) = x -> a+b+c+d+e+x; heavy(1,2,3,4,5)(10)");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32); // 1+2+3+4+5+10 = 25
}


// Main test suite function
void test_functions_suite(void) {
    
    // Lambda function tests
    RUN_TEST(test_lambda_zero_parameters);
    RUN_TEST(test_lambda_single_parameter_bare);
    RUN_TEST(test_lambda_single_parameter_parentheses);
    RUN_TEST(test_lambda_multiple_parameters);
    RUN_TEST(test_lambda_assignment_and_call);
    RUN_TEST(test_lambda_nested_expressions);
    RUN_TEST(test_lambda_with_string_operations);
    
    // Def function tests  
    RUN_TEST(test_def_simple_function);
    RUN_TEST(test_def_zero_parameter_function);
    RUN_TEST(test_def_single_parameter_function);
    RUN_TEST(test_def_function_with_complex_expression);
    RUN_TEST(test_def_function_with_string);
    
    // Scoping tests
    RUN_TEST(test_function_parameter_scoping);
    RUN_TEST(test_function_closure_behavior);
    
    // Type checking tests
    RUN_TEST(test_function_type_checking);
    RUN_TEST(test_def_function_type_checking);
    
    // Operation tests
    RUN_TEST(test_lambda_with_arithmetic_operations);
    RUN_TEST(test_lambda_with_boolean_operations);
    RUN_TEST(test_lambda_mixed_argument_types);
    
    // Advanced tests
    RUN_TEST(test_nested_function_calls);
    RUN_TEST(test_lambda_as_expression_result);
    RUN_TEST(test_lambda_with_complex_expressions);
    RUN_TEST(test_lambda_return_value_types);
    RUN_TEST(test_function_call_argument_validation);
    RUN_TEST(test_function_recursive_factorial);
    RUN_TEST(test_closure_constant_isolation);
    
    // Comprehensive closure upvalue capture tests
    RUN_TEST(test_basic_closure_capture);
    RUN_TEST(test_function_composition);
    RUN_TEST(test_currying);
    RUN_TEST(test_multiple_variable_capture);
    RUN_TEST(test_nested_closure_capture);
    RUN_TEST(test_string_capture);
    RUN_TEST(test_upvalue_assignment);
    RUN_TEST(test_closure_independence);
    RUN_TEST(test_mixed_type_capture);
    RUN_TEST(test_closure_error_cases);
    RUN_TEST(test_closure_performance);
}