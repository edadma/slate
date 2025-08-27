#include "unity.h"
#include "vm.h"
#include "builtins.h"
#include "parser.h"
#include "codegen.h"
#include "lexer.h"

// Helper function to interpret a single expression and return result
static value_t interpret_expression(const char* source) {
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
        // Retain result before cleanup
        return_value = vm_retain(return_value);
    }
    
    // Cleanup (don't call function_destroy - codegen owns the function)
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return return_value;
}


// Test print function (returns undefined)
void test_builtin_print(void) {
    value_t result = interpret_expression("print(42)");
    TEST_ASSERT_EQUAL(VAL_UNDEFINED, result.type);
    vm_release(result);
}

// Test type function for numeric types
void test_builtin_type_number(void) {
    // Test int32 type
    value_t result = interpret_expression("type(42)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("int32", result.as.string);
    vm_release(result);
    
    // Test number (float) type
    result = interpret_expression("type(3.14)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("number", result.as.string);
    vm_release(result);
}

void test_builtin_type_string(void) {
    value_t result = interpret_expression("type(\"hello\")");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("string", result.as.string);
    vm_release(result);
}

void test_builtin_type_boolean(void) {
    value_t result = interpret_expression("type(true)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("boolean", result.as.string);
    vm_release(result);
}

void test_builtin_type_null(void) {
    value_t result = interpret_expression("type(null)");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("null", result.as.string);
    vm_release(result);
}

// Test abs function
void test_builtin_abs_positive(void) {
    value_t result = interpret_expression("abs(5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_negative(void) {
    value_t result = interpret_expression("abs(-5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

void test_builtin_abs_zero(void) {
    value_t result = interpret_expression("abs(0)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
}

// Test sqrt function
void test_builtin_sqrt(void) {
    value_t result = interpret_expression("sqrt(16)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(4.0, result.as.number);
    vm_release(result);
}

void test_builtin_sqrt_zero(void) {
    value_t result = interpret_expression("sqrt(0)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);
    vm_release(result);
}

// Test floor function
void test_builtin_floor(void) {
    value_t result = interpret_expression("floor(3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_floor_negative(void) {
    value_t result = interpret_expression("floor(-3.7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-4, result.as.int32);
    vm_release(result);
}

// Test ceil function
void test_builtin_ceil(void) {
    value_t result = interpret_expression("ceil(3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_ceil_negative(void) {
    value_t result = interpret_expression("ceil(-3.2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-3, result.as.int32);
    vm_release(result);
}

// Test round function
void test_builtin_round_up(void) {
    value_t result = interpret_expression("round(3.6)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

void test_builtin_round_down(void) {
    value_t result = interpret_expression("round(3.4)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_round_half(void) {
    value_t result = interpret_expression("round(3.5)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
}

// Test min function
void test_builtin_min(void) {
    value_t result = interpret_expression("min(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
}

void test_builtin_min_negative(void) {
    value_t result = interpret_expression("min(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-5, result.as.int32);
    vm_release(result);
}

// Test max function
void test_builtin_max(void) {
    value_t result = interpret_expression("max(3, 7)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(7, result.as.int32);
    vm_release(result);
}

void test_builtin_max_negative(void) {
    value_t result = interpret_expression("max(-5, -2)");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(-2, result.as.int32);
    vm_release(result);
}

// Test random function (just check it returns a number in valid range)
void test_builtin_random(void) {
    value_t result = interpret_expression("random()");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_TRUE(result.as.number >= 0.0);
    TEST_ASSERT_TRUE(result.as.number <= 1.0);
    vm_release(result);
}

// Test sin function
void test_builtin_sin_zero(void) {
    value_t result = interpret_expression("sin(0)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);
    vm_release(result);
}

void test_builtin_sin_pi_half(void) {
    value_t result = interpret_expression("sin(3.14159265359/2)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.0, result.as.number);
    vm_release(result);
}

// Test cos function
void test_builtin_cos_zero(void) {
    value_t result = interpret_expression("cos(0)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(1.0, result.as.number);
    vm_release(result);
}

void test_builtin_cos_pi(void) {
    value_t result = interpret_expression("cos(3.14159265359)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, -1.0, result.as.number);
    vm_release(result);
}

// Test tan function
void test_builtin_tan_zero(void) {
    value_t result = interpret_expression("tan(0)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_EQUAL_DOUBLE(0.0, result.as.number);
    vm_release(result);
}

void test_builtin_tan_pi_quarter(void) {
    value_t result = interpret_expression("tan(3.14159265359/4)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 1.0, result.as.number);
    vm_release(result);
}

// Test trig functions with integer arguments
void test_builtin_sin_integer(void) {
    value_t result = interpret_expression("sin(1)");
    TEST_ASSERT_EQUAL(VAL_NUMBER, result.type);
    TEST_ASSERT_DOUBLE_WITHIN(0.000001, 0.841471, result.as.number);
    vm_release(result);
}

// Test string concatenation with arrays
void test_string_concat_with_array(void) {
    value_t result = interpret_expression("\"Array: \" + [1, 2, 3]");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Array: [1, 2, 3]", result.as.string);
    vm_release(result);
}

void test_string_concat_with_empty_array(void) {
    value_t result = interpret_expression("\"Empty: \" + []");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Empty: []", result.as.string);
    vm_release(result);
}

void test_string_concat_with_nested_array(void) {
    value_t result = interpret_expression("\"Nested: \" + [[1, 2], [3, 4]]");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Nested: [[1, 2], [3, 4]]", result.as.string);
    vm_release(result);
}

// Test string concatenation with objects
void test_string_concat_with_object(void) {
    value_t result = interpret_expression("\"Object: \" + {name: \"Test\", value: 42}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    // Note: Object property order might vary
    TEST_ASSERT_TRUE(strstr(result.as.string, "Object: {") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "name: \"Test\"") != NULL);
    TEST_ASSERT_TRUE(strstr(result.as.string, "value: 42") != NULL);
    vm_release(result);
}

void test_string_concat_with_empty_object(void) {
    value_t result = interpret_expression("\"Empty: \" + {}");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Empty: {}", result.as.string);
    vm_release(result);
}

// Test array with mixed types including strings
void test_array_with_strings(void) {
    value_t result = interpret_expression("[1, \"hello\", true, null]");
    TEST_ASSERT_EQUAL(VAL_ARRAY, result.type);
    // Convert to string to check display format
    value_t str_result = interpret_expression("\"\" + [1, \"hello\", true, null]");
    TEST_ASSERT_EQUAL(VAL_STRING, str_result.type);
    TEST_ASSERT_EQUAL_STRING("[1, \"hello\", true, null]", str_result.as.string);
    vm_release(result);
    vm_release(str_result);
}

// Test object with string values
void test_object_with_string_values(void) {
    value_t result = interpret_expression("{greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_OBJECT, result.type);
    // Convert to string to check display format
    value_t str_result = interpret_expression("\"\" + {greeting: \"hello\", name: \"world\"}");
    TEST_ASSERT_EQUAL(VAL_STRING, str_result.type);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "greeting: \"hello\"") != NULL);
    TEST_ASSERT_TRUE(strstr(str_result.as.string, "name: \"world\"") != NULL);
    vm_release(result);
    vm_release(str_result);
}

// ==================
// BUFFER UNIT TESTS
// ==================

// Test buffer creation from string
void test_buffer_creation_from_string(void) {
    value_t result = interpret_expression("buffer(\"Hello\")");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    vm_release(result);
}

// Test buffer creation from array
void test_buffer_creation_from_array(void) {
    value_t result = interpret_expression("buffer([72, 101, 108, 108, 111])");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    // Check if it represents "Hello"
    TEST_ASSERT_EQUAL(72, ((uint8_t*)result.as.buffer)[0]); // 'H'
    TEST_ASSERT_EQUAL(101, ((uint8_t*)result.as.buffer)[1]); // 'e'
    vm_release(result);
}

// Test buffer from hex string
void test_buffer_from_hex(void) {
    value_t result = interpret_expression("buffer_from_hex(\"48656c6c6f\")");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    // Should represent "Hello"
    TEST_ASSERT_EQUAL(0, memcmp(result.as.buffer, "Hello", 5));
    vm_release(result);
}

// Test buffer to hex conversion
void test_buffer_to_hex(void) {
    value_t result = interpret_expression("buffer_to_hex(buffer(\"Hello\"))");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("48656c6c6f", result.as.string);
    vm_release(result);
}

// Test buffer slicing
void test_buffer_slice(void) {
    value_t result = interpret_expression("buffer_to_hex(buffer_slice(buffer(\"Hello\"), 1, 3))");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("656c6c", result.as.string); // "ell" in hex
    vm_release(result);
}

// Test buffer concatenation
void test_buffer_concat(void) {
    value_t result = interpret_expression("buffer_to_hex(buffer_concat(buffer(\"Hello\"), buffer(\" World\")))");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("48656c6c6f20576f726c64", result.as.string); // "Hello World" in hex
    vm_release(result);
}

// Test buffer type checking
void test_buffer_type_checking(void) {
    value_t result = interpret_expression("type(buffer(\"test\"))");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("buffer", result.as.string);
    vm_release(result);
}

// Test buffer reader basic functionality
void test_buffer_reader_basic(void) {
    value_t result = interpret_expression("reader_read_uint8(buffer_reader(buffer(\"H\")))");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(72, result.as.int32); // ASCII 'H'
    vm_release(result);
}

// Test buffer reader positioning
void test_buffer_reader_positioning(void) {
    // Test remaining bytes
    value_t result = interpret_expression("reader_remaining(buffer_reader(buffer(\"Hello\")))");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32);
    vm_release(result);
    
    // Test position
    result = interpret_expression("reader_position(buffer_reader(buffer(\"Hello\")))");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

// Test suite function
void test_builtins_suite(void) {
    RUN_TEST(test_builtin_print);
    RUN_TEST(test_builtin_type_number);
    RUN_TEST(test_builtin_type_string);
    RUN_TEST(test_builtin_type_boolean);
    RUN_TEST(test_builtin_type_null);
    RUN_TEST(test_builtin_abs_positive);
    RUN_TEST(test_builtin_abs_negative);
    RUN_TEST(test_builtin_abs_zero);
    RUN_TEST(test_builtin_sqrt);
    RUN_TEST(test_builtin_sqrt_zero);
    RUN_TEST(test_builtin_floor);
    RUN_TEST(test_builtin_floor_negative);
    RUN_TEST(test_builtin_ceil);
    RUN_TEST(test_builtin_ceil_negative);
    RUN_TEST(test_builtin_round_up);
    RUN_TEST(test_builtin_round_down);
    RUN_TEST(test_builtin_round_half);
    RUN_TEST(test_builtin_min);
    RUN_TEST(test_builtin_min_negative);
    RUN_TEST(test_builtin_max);
    RUN_TEST(test_builtin_max_negative);
    RUN_TEST(test_builtin_random);
    RUN_TEST(test_builtin_sin_zero);
    RUN_TEST(test_builtin_sin_pi_half);
    RUN_TEST(test_builtin_cos_zero);
    RUN_TEST(test_builtin_cos_pi);
    RUN_TEST(test_builtin_tan_zero);
    RUN_TEST(test_builtin_tan_pi_quarter);
    RUN_TEST(test_builtin_sin_integer);
    
    // String concatenation tests
    RUN_TEST(test_string_concat_with_array);
    RUN_TEST(test_string_concat_with_empty_array);
    RUN_TEST(test_string_concat_with_nested_array);
    RUN_TEST(test_string_concat_with_object);
    RUN_TEST(test_string_concat_with_empty_object);
    RUN_TEST(test_array_with_strings);
    RUN_TEST(test_object_with_string_values);
    
    // Buffer tests
    RUN_TEST(test_buffer_creation_from_string);
    RUN_TEST(test_buffer_creation_from_array);
    RUN_TEST(test_buffer_from_hex);
    RUN_TEST(test_buffer_to_hex);
    RUN_TEST(test_buffer_slice);
    RUN_TEST(test_buffer_concat);
    RUN_TEST(test_buffer_type_checking);
    RUN_TEST(test_buffer_reader_basic);
    RUN_TEST(test_buffer_reader_positioning);
}