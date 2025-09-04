#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>


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
    value_t result = test_execute_expression("String(65)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("A", result.as.string);
    vm_release(result);
}

void test_string_factory_multiple_codepoints(void) {
    value_t result = test_execute_expression("String(72, 101, 108, 108, 111)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello", result.as.string);
    vm_release(result);
}

void test_string_factory_array_of_codepoints(void) {
    value_t result = test_execute_expression("String([72, 105])");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hi", result.as.string);
    vm_release(result);
}

void test_string_factory_empty(void) {
    value_t result = test_execute_expression("String()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
}

void test_string_factory_empty_array(void) {
    value_t result = test_execute_expression("String([])");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
}

void test_string_factory_unicode_emoji(void) {
    value_t result = test_execute_expression("String(128512)"); // 😀 emoji
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("😀", result.as.string);
    vm_release(result);
}

void test_string_factory_unicode_array(void) {
    value_t result = test_execute_expression("String([128512, 32, 128515])"); // "😀 😃"
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("😀 😃", result.as.string);
    vm_release(result);
}

void test_string_factory_mixed_ascii_unicode(void) {
    value_t result = test_execute_expression("String(72, 105, 32, 128512)"); // "Hi 😀"
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hi 😀", result.as.string);
    vm_release(result);
}

void test_string_factory_special_chars(void) {
    value_t result = test_execute_expression("String(9, 10, 13)"); // Tab, newline, carriage return
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("\t\n\r", result.as.string);
    vm_release(result);
}

void test_string_factory_max_valid_codepoint(void) {
    value_t result = test_execute_expression("String(1114111)"); // 0x10FFFF - max valid Unicode
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    // The result should be a valid 4-byte UTF-8 sequence
    TEST_ASSERT_EQUAL_INT(4, strlen(result.as.string));
    vm_release(result);
}

// =============================================================================
// STRING METHOD TESTS
// =============================================================================

void test_string_length(void) {
    value_t result = test_execute_expression("\"hello\".length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("\"\".length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

void test_string_substring(void) {
    value_t result = test_execute_expression("\"Hello World\".substring(0, 5)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".substring(6, 5)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("World", result.as.string);
    vm_release(result);
}

void test_string_to_upper(void) {
    value_t result = test_execute_expression("\"hello world\".toUpper()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"HeLLo\".toUpper()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("HELLO", result.as.string);
    vm_release(result);
}

void test_string_to_lower(void) {
    value_t result = test_execute_expression("\"HELLO WORLD\".toLower()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello world", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"HeLLo\".toLower()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
}

void test_string_trim(void) {
    value_t result = test_execute_expression("\"  hello  \".trim()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("hello", result.as.string);
    vm_release(result);
    
    // Test with spaces and tabs
    result = test_execute_expression("\"   test   \".trim()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("test", result.as.string);
    vm_release(result);
}

void test_string_starts_with(void) {
    value_t result = test_execute_expression("\"Hello World\".startsWith(\"Hello\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".startsWith(\"World\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
}

void test_string_ends_with(void) {
    value_t result = test_execute_expression("\"Hello World\".endsWith(\"World\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".endsWith(\"Hello\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
}

void test_string_contains(void) {
    value_t result = test_execute_expression("\"Hello World\".contains(\"lo Wo\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".contains(\"xyz\")");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
}

void test_string_replace(void) {
    value_t result = test_execute_expression("\"Hello World\".replace(\"World\", \"Universe\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello Universe", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".replace(\"xyz\", \"abc\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);
}

void test_string_index_of(void) {
    value_t result = test_execute_expression("\"Hello World\".indexOf(\"World\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(6, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".indexOf(\"o\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(4, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("\"Hello World\".indexOf(\"xyz\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(-1, result.as.int32);
    vm_release(result);
}

void test_string_method_chaining(void) {
    value_t result = test_execute_expression("\"  hello world  \".trim().toUpper()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("HELLO WORLD", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("\"HELLO\".toLower().replace(\"h\", \"j\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("jello", result.as.string);
    vm_release(result);
}

void test_string_is_empty_non_empty(void) {
    value_t result = test_execute_expression("\"\".isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"\".nonEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"hello\".isEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(0, result.as.boolean);
    vm_release(result);
    
    result = test_execute_expression("\"hello\".nonEmpty()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL(1, result.as.boolean);
    vm_release(result);
}

// =============================================================================
// STRING CONCATENATION TESTS
// =============================================================================

void test_string_concat_with_array(void) {
    value_t result = test_execute_expression("\"Array: \" + [1, 2, 3]");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Array: [1, 2, 3]", result.as.string);
    vm_release(result);
}

void test_string_concat_with_empty_array(void) {
    value_t result = test_execute_expression("\"Empty: \" + []");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Empty: []", result.as.string);
    vm_release(result);
}

void test_string_concat_with_nested_array(void) {
    value_t result = test_execute_expression("\"Nested: \" + [[1, 2], [3, 4]]");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Nested: [[1, 2], [3, 4]]", result.as.string);
    vm_release(result);
}

void test_string_concat_with_object(void) {
    value_t result = test_execute_expression("\"Object: \" + {name: \"Test\", value: 42}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    // Note: Object property order might vary
    TEST_ASSERT_TRUE(strstr(result.as.string, "Object: {") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "name: \"Test\"") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "value: 42") != NULL);
    vm_release(result);
}

void test_string_concat_with_empty_object(void) {
    value_t result = test_execute_expression("\"Empty: \" + {}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Empty: {}", result.as.string);
    vm_release(result);
}

// =============================================================================
// STRING BUILDER TESTS
// =============================================================================

void test_string_builder_creation_empty(void) {
    value_t result = test_execute_expression("StringBuilder()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING_BUILDER, result.type);
    vm_release(result);
}

void test_string_builder_creation_with_capacity(void) {
    value_t result = test_execute_expression("StringBuilder(100)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING_BUILDER, result.type);
    vm_release(result);
}

void test_string_builder_creation_with_strings(void) {
    value_t result = test_execute_expression("StringBuilder(\"Hello\", \" \", \"World\")");
    TEST_ASSERT_EQUAL_INT(VAL_STRING_BUILDER, result.type);
    vm_release(result);
}

void test_string_builder_creation_with_capacity_and_strings(void) {
    value_t result = test_execute_expression("StringBuilder(50, \"Start\", \" here\")");
    TEST_ASSERT_EQUAL_INT(VAL_STRING_BUILDER, result.type);
    vm_release(result);
}

void test_string_builder_append(void) {
    value_t result = test_execute_expression("var sb = StringBuilder(); sb.append(\"Hello\"); sb.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello", result.as.string);
    vm_release(result);
}

void test_string_builder_append_chaining(void) {
    value_t result = test_execute_expression("StringBuilder().append(\"Hello\").append(\" \").append(\"World\").toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);
}

void test_string_builder_append_char(void) {
    value_t result = test_execute_expression("StringBuilder().appendChar(72).appendChar(101).appendChar(108).appendChar(108).appendChar(111).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello", result.as.string);
    vm_release(result);
}

void test_string_builder_append_char_unicode(void) {
    value_t result = test_execute_expression("StringBuilder().appendChar(128512).toString()"); // 😀 emoji
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("😀", result.as.string);
    vm_release(result);
}

void test_string_builder_length(void) {
    value_t result = test_execute_expression("StringBuilder(\"Hello World\").length()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(11, result.as.int32);
    vm_release(result);
    
    result = test_execute_expression("var sb = StringBuilder(); sb.append(\"Test\"); sb.length()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(4, result.as.int32);
    vm_release(result);
}

void test_string_builder_clear(void) {
    value_t result = test_execute_expression("var sb = StringBuilder(\"Hello\"); sb.clear(); sb.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("", result.as.string);
    vm_release(result);
    
    result = test_execute_expression("var sb = StringBuilder(\"Hello\"); sb.clear(); sb.length()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

void test_string_builder_mixed_operations(void) {
    value_t result = test_execute_expression("var sb = StringBuilder(); sb.append(\"Count: \"); sb.appendChar(49); sb.append(\", \"); sb.appendChar(50); sb.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Count: 1, 2", result.as.string);
    vm_release(result);
}

void test_string_builder_initial_content(void) {
    value_t result = test_execute_expression("StringBuilder(\"Pre\", \"-\", \"filled\").toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Pre-filled", result.as.string);
    vm_release(result);
}

void test_string_builder_capacity_with_content(void) {
    value_t result = test_execute_expression("StringBuilder(100, \"Big\", \" \", \"buffer\").toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Big buffer", result.as.string);
    vm_release(result);
}

// =============================================================================
// TEST SUITE FUNCTION
// =============================================================================

// Test String.hash() method
void test_string_hash_basic(void) {
    // Test basic string hash
    value_t result = test_execute_expression("hash(\"hello\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_NOT_EQUAL(0, result.as.int32); // Should produce a non-zero hash
    vm_release(result);
    
    result = test_execute_expression("hash(\"world\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_NOT_EQUAL(0, result.as.int32);
    vm_release(result);
}

void test_string_hash_empty(void) {
    // Test empty string hash
    value_t result = test_execute_expression("hash(\"\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    // Empty string should have a specific hash (FNV-1a offset basis)
    vm_release(result);
}

void test_string_hash_consistency(void) {
    // Test that same strings produce same hash
    value_t result1 = test_execute_expression("hash(\"test\")");
    value_t result2 = test_execute_expression("hash(\"test\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_EQUAL_INT32(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
}

void test_string_hash_differences(void) {
    // Test that different strings produce different hashes
    value_t result1 = test_execute_expression("hash(\"abc\")");
    value_t result2 = test_execute_expression("hash(\"def\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_NOT_EQUAL(result1.as.int32, result2.as.int32);
    vm_release(result1);
    vm_release(result2);
    
    // Test similar strings
    result1 = test_execute_expression("hash(\"test\")");
    result2 = test_execute_expression("hash(\"Test\")");
    TEST_ASSERT_EQUAL(VAL_INT32, result1.type);
    TEST_ASSERT_EQUAL(VAL_INT32, result2.type);
    TEST_ASSERT_NOT_EQUAL(result1.as.int32, result2.as.int32); // Case sensitive
    vm_release(result1);
    vm_release(result2);
}

void test_string_method_hash_equality(void) {
    // Test that identical strings have equal hash via method call
    value_t result = test_execute_expression("\"hello\".hash() == \"hello\".hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(1, result.as.boolean);
    vm_release(result);
    
    // Test that different strings have different hashes
    result = test_execute_expression("\"hello\".hash() == \"world\".hash()");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_EQUAL_INT(0, result.as.boolean);
    vm_release(result);
}

void test_class_string_suite(void) {
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
    
    // String method tests (moved from test_builtins.c)
    RUN_TEST(test_string_length);
    RUN_TEST(test_string_substring);
    RUN_TEST(test_string_to_upper);
    RUN_TEST(test_string_to_lower);
    RUN_TEST(test_string_trim);
    RUN_TEST(test_string_starts_with);
    RUN_TEST(test_string_ends_with);
    RUN_TEST(test_string_contains);
    RUN_TEST(test_string_replace);
    RUN_TEST(test_string_index_of);
    RUN_TEST(test_string_method_chaining);
    RUN_TEST(test_string_is_empty_non_empty);
    
    // String concatenation tests (moved from test_builtins.c)
    RUN_TEST(test_string_concat_with_array);
    RUN_TEST(test_string_concat_with_empty_array);
    RUN_TEST(test_string_concat_with_nested_array);
    RUN_TEST(test_string_concat_with_object);
    RUN_TEST(test_string_concat_with_empty_object);
    
    // StringBuilder tests (new functionality)
    RUN_TEST(test_string_builder_creation_empty);
    RUN_TEST(test_string_builder_creation_with_capacity);
    RUN_TEST(test_string_builder_creation_with_strings);
    RUN_TEST(test_string_builder_creation_with_capacity_and_strings);
    RUN_TEST(test_string_builder_append);
    RUN_TEST(test_string_builder_append_chaining);
    RUN_TEST(test_string_builder_append_char);
    RUN_TEST(test_string_builder_append_char_unicode);
    RUN_TEST(test_string_builder_length);
    RUN_TEST(test_string_builder_clear);
    RUN_TEST(test_string_builder_mixed_operations);
    RUN_TEST(test_string_builder_initial_content);
    RUN_TEST(test_string_builder_capacity_with_content);
    
    // String hash tests
    RUN_TEST(test_string_hash_basic);
    RUN_TEST(test_string_hash_empty);
    RUN_TEST(test_string_hash_consistency);
    RUN_TEST(test_string_hash_differences);
    RUN_TEST(test_string_method_hash_equality);
}