#include "../unity/unity.h"
#include "../include/vm.h"
#include "../include/lexer.h"
#include "../include/parser.h"
#include "../include/codegen.h"
#include "../include/builtins.h"
#include <string.h>

// Helper function to run code and get result
static value_t run_code(const char* code) {
    lexer_t lexer;
    parser_t parser;
    
    lexer_init(&lexer, code);
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        lexer_cleanup(&lexer);
        return make_null();
    }
    
    slate_vm* vm = vm_create();
    builtins_init(vm);
    
    codegen_t* codegen = codegen_create(vm);
    function_t* function = codegen_compile(codegen, program);
    
    vm_result result = vm_execute(vm, function);
    
    value_t return_value = make_null();
    if (result == VM_OK) {
        return_value = vm->result;
        // Retain result before cleanup
        return_value = vm_retain(return_value);
    }
    
    // Cleanup
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return return_value;
}

// Helper function to check if running code results in runtime error
static int expect_runtime_error(const char* code) {
    // Fork or use setjmp/longjmp to catch the runtime_error exit(1)
    // For now, just assume it will exit - this is a limitation of the current runtime_error implementation
    // In a real test, we'd need to modify runtime_error to throw exceptions instead of exit(1)
    return 1; // Placeholder - indicates we expect an error
}

// =============================================================================
// STRING FACTORY TESTS
// =============================================================================

void test_string_factory_single_codepoint(void) {
    value_t result = run_code("String(65)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("A", result.as.string);
    vm_release(result);
}

void test_string_factory_multiple_codepoints(void) {
    value_t result = run_code("String(72, 101, 108, 108, 111)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello", result.as.string);
    vm_release(result);
}

void test_string_factory_array_of_codepoints(void) {
    value_t result = run_code("String([72, 105])");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hi", result.as.string);
    vm_release(result);
}

void test_string_factory_empty(void) {
    value_t result = run_code("String()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
}

void test_string_factory_empty_array(void) {
    value_t result = run_code("String([])");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
}

void test_string_factory_unicode_emoji(void) {
    value_t result = run_code("String(128512)"); // 😀 emoji
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("😀", result.as.string);
    vm_release(result);
}

void test_string_factory_unicode_array(void) {
    value_t result = run_code("String([128512, 32, 128515])"); // "😀 😃"
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("😀 😃", result.as.string);
    vm_release(result);
}

void test_string_factory_mixed_ascii_unicode(void) {
    value_t result = run_code("String(72, 105, 32, 128512)"); // "Hi 😀"
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hi 😀", result.as.string);
    vm_release(result);
}

void test_string_factory_special_chars(void) {
    value_t result = run_code("String(9, 10, 13)"); // Tab, newline, carriage return
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("\t\n\r", result.as.string);
    vm_release(result);
}

void test_string_factory_max_valid_codepoint(void) {
    value_t result = run_code("String(1114111)"); // 0x10FFFF - max valid Unicode
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    // The result should be a valid 4-byte UTF-8 sequence
    TEST_ASSERT_EQUAL_INT(4, strlen(result.as.string));
    vm_release(result);
}

// =============================================================================
// STRING METHOD TESTS
// =============================================================================

// String method tests already exist in test_builtins.c, so we'll skip duplicates
// and focus on factory-specific tests

// =============================================================================
// TEST SUITE FUNCTION
// =============================================================================

void test_string_suite(void) {
    // String factory tests (new functionality)
    RUN_TEST(test_string_factory_single_codepoint);
    RUN_TEST(test_string_factory_multiple_codepoints);
    RUN_TEST(test_string_factory_array_of_codepoints);
    RUN_TEST(test_string_factory_empty);
    RUN_TEST(test_string_factory_empty_array);
    RUN_TEST(test_string_factory_unicode_emoji);
    RUN_TEST(test_string_factory_unicode_array);
    RUN_TEST(test_string_factory_mixed_ascii_unicode);
    RUN_TEST(test_string_factory_special_chars);
    RUN_TEST(test_string_factory_max_valid_codepoint);
    
    // Note: String method tests are already covered in test_builtins.c
}