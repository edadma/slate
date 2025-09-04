#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "ast.h"

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
                      "Some";  // Try to access the constructor
    value_t result = test_execute_expression(code);
    // Should not fail with "undefined variable" error
    // Now returns a class because constructors work properly
    TEST_ASSERT_EQUAL(VAL_CLASS, result.type);
    vm_release(result);
}

// Test empty data type constructor access
void test_data_empty_constructor_access(void) {
    const char* code = "data Widget\n"
                      "Widget";  // Try to access the empty data type constructor
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
                      "a == b";  // Different constructors should be different
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
                      "Person";  // Try to access the constructor
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

// Test data type with methods (placeholder for future)
void test_data_with_methods_parsing(void) {
    // Methods within data types aren't fully implemented yet
    // Currently this causes a parse error, which is expected
    const char* code = "data Option\n"
                      "  def getOrElse(default) = null\n"  // Not yet supported
                      "  case Some(value)\n"
                      "  case None";
    bool error_occurred = test_expect_parse_error(code);
    TEST_ASSERT_TRUE(error_occurred);  // Currently fails to parse, which is expected
}

// ===========================
// ERROR HANDLING TESTS
// ===========================

// Helper function to test for parse errors
static bool test_expect_parse_error(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    bool had_error = parser.had_error;
    
    // Clean up
    if (program) {
        ast_free((ast_node*)program);
    }
    lexer_cleanup(&lexer);
    
    return had_error;
}

// Test invalid data declaration syntax
void test_data_invalid_syntax(void) {
    // Missing data type name
    bool error_occurred = test_expect_parse_error("data");
    TEST_ASSERT_TRUE(error_occurred);
}

// Test invalid case syntax
void test_data_invalid_case_syntax(void) {
    // Case without name
    const char* code = "data Option\n"
                      "  case";
    bool error_occurred = test_expect_parse_error(code);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test private without data
void test_data_private_without_data(void) {
    bool error_occurred = test_expect_parse_error("private var x = 5");
    TEST_ASSERT_TRUE(error_occurred);
}

// Test mismatched end marker name
void test_data_mismatched_end_marker(void) {
    const char* code = "data Option\n"
                      "  case Some(value)\n" 
                      "  case None\n"
                      "end Result";  // Wrong name
    bool error_occurred = test_expect_parse_error(code);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test end without identifier
void test_data_end_without_name(void) {
    const char* code = "data Option\n"
                      "  case Some(value)\n"
                      "  case None\n"
                      "end";  // Missing name
    bool error_occurred = test_expect_parse_error(code);
    TEST_ASSERT_TRUE(error_occurred);
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
    RUN_TEST(test_data_with_methods_parsing);
    
    RUN_TEST(test_data_invalid_syntax);
    RUN_TEST(test_data_invalid_case_syntax);
    RUN_TEST(test_data_private_without_data);
    RUN_TEST(test_data_mismatched_end_marker);
    RUN_TEST(test_data_end_without_name);
}