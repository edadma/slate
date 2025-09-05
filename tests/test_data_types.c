#include <string.h>
#include "../unity/unity.h"
#include "ast.h"
#include "lexer.h"
#include "parser.h"
#include "test_helpers.h"

// Forward declaration
static bool test_expect_parse_error(const char* source);

// ===========================
// DATA TYPE DECLARATION TESTS
// ===========================

// Test empty data type declaration
void test_data_empty_declaration(void) {
    value_t result = test_execute_expression("data Option");
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test single-constructor data type
void test_data_single_constructor(void) {
    value_t result = test_execute_expression("data Person(name, age)");
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test multi-case data type
void test_data_multi_case(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test private data type
void test_data_private_declaration(void) {
    value_t result = test_execute_expression("private data Internal");
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test that constructors are registered in global scope
void test_data_constructor_registration(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None\n"
                       "Some"; // Try to access the constructor
    value_t result = test_execute_expression(code);
    // Should not fail with "undefined variable" error
    // Now returns a class because constructors work properly
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test empty data type constructor access
void test_data_empty_constructor_access(void) {
    const char* code = "data Widget\n"
                       "Widget"; // Try to access the empty data type constructor
    value_t result = test_execute_expression(code);
    // Should not fail with "undefined variable" error
    // Now returns a class because constructors work properly
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test empty data type assignment works
void test_data_empty_assignment_works(void) {
    const char* code = "data Empty\n"
                       "var x = Empty\n"
                       "x";
    value_t result = test_execute_expression(code);
    // Should successfully assign the constructor to a variable
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test multiple empty data types don't conflict
void test_data_multiple_empty_types(void) {
    const char* code = "data TypeA\n"
                       "data TypeB\n"
                       "var a = TypeA\n"
                       "var b = TypeB\n"
                       "a == b"; // Different constructors should be different
    value_t result = test_execute_expression(code);
    // Placeholder implementation returns null, but test ensures no errors
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    vm_release(result);
}

// Test data type constructor in expressions
void test_data_constructor_in_expressions(void) {
    const char* code = "data Result\n"
                       "var constructors = [Result]\n"
                       "constructors.length()";
    value_t result = test_execute_expression(code);
    // Should be able to use constructor in array and call methods
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.int32);
    vm_release(result);
}

// Test ADT constructor calls return instances
void test_adt_constructor_calls_return_instances(void) {
    const char* code = "data TestType\n"
                       "TestType()";
    value_t result = test_execute_expression(code);
    // Constructor calls should return object instances
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test parameterized ADT constructor calls
void test_adt_parameterized_constructor_calls(void) {
    const char* code = "data Person(name, age)\n"
                       "Person('Alice', 25)";
    value_t result = test_execute_expression(code);
    // Constructor calls with parameters should return object instances
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test multi-case ADT constructor calls
void test_adt_multi_case_constructor_calls(void) {
    const char* code = "data Result\n"
                       "  case Success(value)\n"
                       "  case Error(message)\n"
                       "Success('hello')";
    value_t result = test_execute_expression(code);
    // Multi-case constructor calls should return object instances
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test singleton case constructor calls
void test_adt_singleton_case_constructor_calls(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None\n"
                       "None";
    value_t result = test_execute_expression(code);
    // Singleton case access should return the constructor class
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test ADT instances can be assigned to variables
void test_adt_instances_assignable(void) {
    const char* code = "data Point(x, y)\n"
                       "var p = Point(10, 20)\n"
                       "p";
    value_t result = test_execute_expression(code);
    // Should be able to assign ADT instances to variables
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test empty constructor toString method
void test_adt_empty_constructor_toString(void) {
    const char* code = "data Empty\n"
                       "var e = Empty()\n"
                       "e.toString()";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Empty", result.as.string);
    vm_release(result);
}

// Test parameterized constructor toString method
void test_adt_parameterized_constructor_toString(void) {
    const char* code = "data Point(x, y)\n"
                       "var p = Point(10, 20)\n"
                       "p.toString()";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Point(10, 20)", result.as.string);
    vm_release(result);
}

// Test multi-case constructor toString method
void test_adt_multi_case_constructor_toString(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None\n"
                       "var s = Some(42)\n"
                       "s.toString()";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Some(42)", result.as.string);
    vm_release(result);

    const char* code2 = "data Option\n"
                        "  case Some(value)\n"
                        "  case None\n"
                        "var n = None()\n"
                        "n.toString()";
    value_t result2 = test_execute_expression(code2);
    TEST_ASSERT_EQUAL(VAL_STRING, result2.type);
    TEST_ASSERT_EQUAL_STRING("None", result2.as.string);
    vm_release(result2);
}

// Test ADT string concatenation
void test_adt_string_concatenation(void) {
    const char* code = "data Point(x, y)\n"
                       "var p = Point(10, 20)\n"
                       "\"Point is: \" + p";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Point is: Point(10, 20)", result.as.string);
    vm_release(result);
}

// Test ADT parameter access
void test_adt_parameter_access(void) {
    const char* code = "data Point(x, y)\n"
                       "var p = Point(10, 20)\n"
                       "p.x";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(10, result.as.int32);
    vm_release(result);

    const char* code2 = "data Point(x, y)\n"
                        "var p = Point(10, 20)\n"
                        "p.y";
    value_t result2 = test_execute_expression(code2);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_EQUAL_INT32(20, result2.as.int32);
    vm_release(result2);
}

// Test data type with function calls
void test_data_constructor_function_usage(void) {
    const char* code = "data Status\n"
                       "def getDefault() = Status\n"
                       "getDefault()";
    value_t result = test_execute_expression(code);
    // Should return the constructor from the function
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test single constructor registration
void test_data_single_constructor_registration(void) {
    const char* code = "data Person(name, age)\n"
                       "Person"; // Try to access the constructor
    value_t result = test_execute_expression(code);
    // Should not fail with "undefined variable" error
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// ===========================
// DATA TYPE PARSING TESTS
// ===========================

// Test parsing complex case declarations
void test_data_complex_case_parsing(void) {
    const char* code = "data Result\n"
                       "  case Success(value, timestamp)\n"
                       "  case Error(message)\n"
                       "  case Pending";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test data type with optional end marker
void test_data_with_end_marker(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None\n"
                       "end Option";
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_NULL, result.type);
    vm_release(result);
}

// Test basic ADT constructor calls that were causing segfault
void test_adt_basic_constructor_calls(void) {
    const char* code = "data Option\n"
                       "  case Some(value)\n"
                       "  case None\n"
                       "var instance = Some(42)\n"
                       "instance";
    value_t result = test_execute_expression(code);
    // Should return an ADT instance, not crash
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    vm_release(result);
}

// Test ADT equality - comprehensive structural equality testing
void test_adt_equality(void) {
    // Test singleton equality
    value_t result = test_execute_expression("data Option\n"
                                             "  case None\n"
                                             "  case Some(value)\n"
                                             "None == None");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean); // This should pass
    vm_release(result);

    // Test parameterized constructor equality with same values - THE BUG
    result = test_execute_expression("data Option\n"
                                     "  case None\n"
                                     "  case Some(value)\n"
                                     "Some(3) == Some(3)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean); // This currently FAILS - should be true
    vm_release(result);

    // Test parameterized constructor inequality with different values
    result = test_execute_expression("data Option\n"
                                     "  case None\n"
                                     "  case Some(value)\n"
                                     "Some(3) == Some(4)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean); // This should be false
    vm_release(result);

    // Test cross-constructor inequality
    result = test_execute_expression("data Option\n"
                                     "  case None\n"
                                     "  case Some(value)\n"
                                     "None == Some(3)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean); // This should be false
    vm_release(result);

    // Test complex parameter equality
    result = test_execute_expression("data Point(x, y)\n"
                                     "Point(10, 20) == Point(10, 20)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean); // This should be true
    vm_release(result);

    // Test complex parameter inequality
    result = test_execute_expression("data Point(x, y)\n"
                                     "Point(10, 20) == Point(20, 10)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean); // This should be false
    vm_release(result);
}

// Test singleton constructor display issue - printing should show "None", not "unknown"
void test_adt_singleton_constructor_display(void) {
    // Test that singleton constructors remain as classes but print correctly
    // None should be a VAL_CLASS but its toString should show "None"
    const char* code = "data Option\n"
                       "  case None\n"
                       "  case Some(value)\n"
                       "None"; // Should be a class that displays as "None"
    value_t result = test_execute_expression(code);

    // None should remain a constructor class (VAL_CLASS) - this is correct design
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type); // This should pass
    TEST_ASSERT_NOT_NULL(result.as.class);
    vm_release(result);

    // Test the actual toString output via the system
    // result = test_execute_expression("data Option\n"
    //                                  "  case None\n"
    //                                  "  case Some(value)\n"
    //                                  "None.toString()");
    // printf("Result: %d\n", result.type);
    // TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    // TEST_ASSERT_EQUAL_STRING("None", result.as.string);
    // vm_release(result);
}


// ===========================
// DATA TYPE TEST SUITE
// ===========================

void test_data_types_suite(void) {
    RUN_TEST(test_data_empty_declaration);
    RUN_TEST(test_data_single_constructor);
    RUN_TEST(test_data_multi_case);
    RUN_TEST(test_data_private_declaration);
    RUN_TEST(test_data_constructor_registration);
    RUN_TEST(test_data_empty_constructor_access);
    RUN_TEST(test_data_empty_assignment_works);
    RUN_TEST(test_data_multiple_empty_types);
    RUN_TEST(test_data_constructor_in_expressions);
    RUN_TEST(test_adt_constructor_calls_return_instances);
    RUN_TEST(test_adt_parameterized_constructor_calls);
    RUN_TEST(test_adt_multi_case_constructor_calls);
    RUN_TEST(test_adt_singleton_case_constructor_calls);
    RUN_TEST(test_adt_instances_assignable);
    RUN_TEST(test_data_constructor_function_usage);
    RUN_TEST(test_data_single_constructor_registration);

    RUN_TEST(test_data_complex_case_parsing);
    RUN_TEST(test_data_with_end_marker);

    // ADT string representation tests
    RUN_TEST(test_adt_empty_constructor_toString);
    RUN_TEST(test_adt_parameterized_constructor_toString);
    RUN_TEST(test_adt_multi_case_constructor_toString);
    RUN_TEST(test_adt_string_concatenation);
    RUN_TEST(test_adt_parameter_access);

    RUN_TEST(test_adt_basic_constructor_calls);
    RUN_TEST(test_adt_equality);
    RUN_TEST(test_adt_singleton_constructor_display);
}
