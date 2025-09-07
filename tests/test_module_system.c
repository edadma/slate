#include <string.h>
#include <stdio.h>
#include "../unity/unity.h"
#include "test_helpers.h"
#include "module.h"
#include "runtime_error.h"

// ===========================
// BASIC IMPORT/EXPORT TESTS
// ===========================

// Test importing val declarations
void test_import_val_declarations(void) {
    // First test if our test module file exists
    char* module_path = test_get_module_path("declarations");
    TEST_ASSERT_NOT_NULL(module_path);
    TEST_ASSERT_TRUE(module_file_exists(module_path));
    free(module_path);
    
    // Test importing a constant value
    const char* code = 
        "import declarations.{CONSTANT_VALUE}\n"
        "CONSTANT_VALUE";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test importing var declarations  
void test_import_var_declarations(void) {
    // Test importing a mutable variable
    const char* code = 
        "import declarations.{mutable_counter}\n"
        "mutable_counter";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

// Test importing def declarations
void test_import_def_declarations(void) {
    // Test importing and calling a function
    const char* code = 
        "import declarations.{square}\n"
        "square(5)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(25, result.as.int32);
    vm_release(result);
}

// Test importing data declarations
void test_import_data_declarations(void) {
    // Test importing and using a data type constructor
    const char* code = 
        "import declarations.{Point}\n"
        "Point(3, 4)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// ===========================
// IMPORT SYNTAX VARIATIONS
// ===========================

// Test wildcard import - imports all declarations
void test_wildcard_import(void) {
    const char* code = 
        "import declarations._\n"
        "CONSTANT_VALUE + square(3)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(51, result.as.int32); // 42 + 9
    vm_release(result);
}

// Test selective import with multiple symbols
void test_selective_import(void) {
    const char* code = 
        "import declarations.{CONSTANT_VALUE, add, PI}\n"
        "add(CONSTANT_VALUE, PI)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_FLOAT_WITHIN(0.001, 45.14159, result.as.float64); // 42 + 3.14159
    vm_release(result);
}

// Test renamed imports
void test_renamed_import(void) {
    const char* code = 
        "import declarations.{CONSTANT_VALUE => const_val, square => sq}\n"
        "sq(const_val)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(1764, result.as.int32); // 42^2 = 1764
    vm_release(result);
}

// ===========================
// PRIVATE DECLARATION TESTS  
// ===========================

// Test that private constants are not exported
void test_private_val_not_exported(void) {
    const char* code = "import private_mixed.{SECRET_CONST}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test that private functions are not exported
void test_private_function_not_exported(void) {
    const char* code = "import private_mixed.{private_helper}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test that private data types are not exported
void test_private_data_not_exported(void) {
    const char* code = "import private_mixed.{InternalState}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test that public declarations from mixed module work
void test_public_from_mixed_module(void) {
    const char* code = 
        "import private_mixed.{PUBLIC_CONST, public_function}\n"
        "public_function(1) + PUBLIC_CONST.length()";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    // public_function returns x * 2, so public_function(1) = 2
    // PUBLIC_CONST is "visible to importers" (20 chars), so length = 20
    TEST_ASSERT_EQUAL(22, result.as.int32); // 2 + 20
    vm_release(result);
}

// ===========================
// COMPLEX DECLARATION TESTS
// ===========================

// Test importing different types of data constructors
void test_data_constructor_variations(void) {
    // Test singleton, multi-case, and single constructor data types
    const char* code = 
        "import datatypes.{Empty, Color, Name}\n"
        "Name(\"John\", \"Doe\").toString()";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_TRUE(strstr(result.as.string, "John") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "Doe") != NULL);
    vm_release(result);
}

// Test function composition across modules
void test_function_composition_across_modules(void) {
    const char* code = 
        "import functions.{compose, multiply, add}\n"
        "var double_then_add_five = compose(x -> add(x, 5), x -> multiply(x, 2))\n"
        "double_then_add_five(3)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(11, result.as.int32); // (3 * 2) + 5 = 11
    vm_release(result);
}

// Test mixing different declaration types in one import
void test_mixed_declaration_import(void) {
    const char* code = 
        "import declarations.{CONSTANT_VALUE, square, Point, Success}\n"
        "Success(Point(CONSTANT_VALUE, square(6)))";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    TEST_ASSERT_NOT_NULL(result.as.object);
    vm_release(result);
}

// ===========================
// ERROR HANDLING TESTS
// ===========================

// Test importing from non-existent module
void test_nonexistent_module_error(void) {
    const char* code = "import nonexistent_module.{foo}";
    
    bool error_occurred = test_expect_import_error(code, ERR_REFERENCE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test importing non-existent symbol from real module
void test_nonexistent_symbol_error(void) {
    const char* code = "import declarations.{nonexistent_symbol}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test circular dependency detection
void test_circular_dependency_error(void) {
    // circular_a imports circular_b which imports circular_a
    const char* code = "import circular_a.{value_from_a}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test importing from module with syntax errors
void test_syntax_error_module(void) {
    const char* code = "import syntax_error.{valid_const}";
    
    bool error_occurred = test_expect_import_error(code, ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// ===========================
// EDGE CASE TESTS
// ===========================

// Test importing from empty module
void test_empty_module_import(void) {
    const char* code = "import empty._; 42";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test importing immutable constants from immutable module
void test_immutable_module_constants(void) {
    const char* code = 
        "import immutable.{NUMBER_CONST, COMPUTED, DERIVED}\n"
        "NUMBER_CONST + COMPUTED + DERIVED";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(203, result.as.int32); // 100 + 53 + 50 = 203
    vm_release(result);
}

// Test function calls with imported data types
void test_functions_with_imported_data(void) {
    const char* code = 
        "import functions.{maximum}\n"
        "import immutable.{NUMBER_CONST, DERIVED}\n"
        "maximum(NUMBER_CONST, DERIVED)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(100, result.as.int32); // maximum(100, 50) = 100
    vm_release(result);
}

// Test module namespace access (regression test for module.function() bug)
void test_module_namespace_access(void) {
    // This test reproduces the bug where after importing a module,
    // accessing functions via module.functionName fails with ReferenceError
    // when the function tries to call itself or other functions in the same module
    
    // Test recursive factorial function
    const char* factorial_code = 
        "import recursive_math\n"
        "recursive_math.factorial(5)";
    
    value_t result = test_execute_with_imports(factorial_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(120, result.as.int32); // 5! = 120
    vm_release(result);
    
    // Test recursive GCD function
    const char* gcd_code = 
        "import recursive_math\n" 
        "recursive_math.gcd(48, 18)";
    
    result = test_execute_with_imports(gcd_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(6, result.as.int32); // gcd(48, 18) = 6
    vm_release(result);
    
    // Test fibonacci function (double recursion)
    const char* fib_code = 
        "import recursive_math\n" 
        "recursive_math.fibonacci(6)";
    
    result = test_execute_with_imports(fib_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(8, result.as.int32); // fib(6) = 8
    vm_release(result);
    
    // Test function that calls another function in the same module
    const char* mixed_code = 
        "import recursive_math\n" 
        "recursive_math.factorial_and_gcd(3, 12, 8)";
    
    result = test_execute_with_imports(mixed_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(10, result.as.int32); // factorial(3) + gcd(12, 8) = 6 + 4 = 10
    vm_release(result);
}

// Test namespace import for deeply nested modules
void test_deep_nested_namespace_import(void) {
    // This test verifies that deeply nested module paths are correctly treated as
    // namespace imports rather than being split into parent.item imports
    //
    // Before the fix: "import submodules.deeply_nested" would be parsed as
    // importing "deeply_nested" from "submodules" (which doesn't exist)
    //
    // After the fix: The heuristic recognizes multi-segment paths
    // and treats them as namespace imports
    
    // Test importing the deeply nested module as a namespace
    const char* code = 
        "import submodules.deeply_nested\n"
        "deeply_nested.testFunc(3)";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(6, result.as.int32); // testFunc(3) should be 6
    vm_release(result);
    
    // Test accessing constant from the namespace
    const char* const_code = 
        "import submodules.deeply_nested\n"
        "deeply_nested.testConst";
    
    result = test_execute_with_imports(const_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32); // testConst should be 42
    vm_release(result);
    
    // Test accessing constants from the namespace
    const char* constant_code = 
        "import examples.modules.math.advanced\n"
        "advanced.PI";
    
    result = test_execute_with_imports(constant_code);
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14159, result.as.float64);
    vm_release(result);
}

// Test that single-item imports still work correctly
void test_single_item_import_still_works(void) {
    // Verify that the namespace import fix didn't break single-item imports
    // Single-item imports should still split at the last dot
    const char* code = 
        "import declarations.CONSTANT_VALUE\n"
        "CONSTANT_VALUE";
    
    value_t result = test_execute_with_imports(code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(42, result.as.int32);
    vm_release(result);
}

// Test single item imports (e.g., import module.item)
void test_single_item_import(void) {
    // Test importing a single function using dot notation
    const char* factorial_code = 
        "import recursive_math.factorial\n"
        "factorial(4)";
    
    value_t result = test_execute_with_imports(factorial_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(24, result.as.int32); // 4! = 24
    vm_release(result);
    
    // Test importing a different single function
    const char* gcd_code = 
        "import recursive_math.gcd\n" 
        "gcd(15, 25)";
    
    result = test_execute_with_imports(gcd_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32); // gcd(15, 25) = 5
    vm_release(result);
    
    // Test importing a constant value using single import
    const char* const_code = 
        "import declarations.PI\n" 
        "PI";
    
    result = test_execute_with_imports(const_code);
    TEST_ASSERT_EQUAL(VAL_FLOAT64, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.001, 3.14159, result.as.float64);
    vm_release(result);
    
    // Test that single import doesn't conflict with namespace access
    const char* mixed_code = 
        "import recursive_math.factorial\n"
        "import recursive_math\n"
        "factorial(3) + recursive_math.gcd(10, 15)";
    
    result = test_execute_with_imports(mixed_code);
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(11, result.as.int32); // factorial(3) + gcd(10, 15) = 6 + 5 = 11
    vm_release(result);
}

// ===========================
// TEST SUITE SETUP
// ===========================

void test_module_system_suite(void) {
    RUN_TEST(test_import_val_declarations);
    RUN_TEST(test_import_var_declarations);
    RUN_TEST(test_import_def_declarations);
    RUN_TEST(test_import_data_declarations);
    
    RUN_TEST(test_wildcard_import);
    RUN_TEST(test_selective_import);
    RUN_TEST(test_renamed_import);
    
    RUN_TEST(test_private_val_not_exported);
    RUN_TEST(test_private_function_not_exported);
    RUN_TEST(test_private_data_not_exported);
    RUN_TEST(test_public_from_mixed_module);
    
    RUN_TEST(test_data_constructor_variations);
    // RUN_TEST(test_function_composition_across_modules); // TODO: Requires module/closure redesign
    RUN_TEST(test_mixed_declaration_import);
    
    // RUN_TEST(test_nonexistent_module_error); // Negative test - disabled for now
    // RUN_TEST(test_nonexistent_symbol_error); // Negative test - disabled for now
    // RUN_TEST(test_circular_dependency_error); // Temporarily disabled - causes crash
    
    RUN_TEST(test_empty_module_import);
    // RUN_TEST(test_immutable_module_constants); // TODO: Requires module/closure redesign
    RUN_TEST(test_functions_with_imported_data);
    
    // Regression test for module namespace access bug
    RUN_TEST(test_module_namespace_access);
    
    // Test for deep nested namespace imports (new feature)
    RUN_TEST(test_deep_nested_namespace_import);
    
    // Test that single-item imports still work after namespace fix
    RUN_TEST(test_single_item_import_still_works);
    
    // Test for single item imports
    RUN_TEST(test_single_item_import);
}