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
        // Retain the result to survive cleanup
        return_value = vm_retain(return_value);
    }
    
    vm_destroy(vm);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    
    return return_value;
}

// ===========================
// BUFFER CLASS CONSTRUCTOR TESTS
// ===========================

// Test Buffer class constructor from string
void test_buffer_class_constructor_string(void) {
    value_t result = run_code("Buffer(\"Hello\")");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    // Verify it's a Buffer object with methods
    TEST_ASSERT_NOT_NULL(result.class);
    vm_release(result);
}

// Test Buffer class constructor from array
void test_buffer_class_constructor_array(void) {
    value_t result = run_code("Buffer([72, 101, 108, 108, 111])");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    // Check if it represents "Hello"
    TEST_ASSERT_EQUAL(72, ((uint8_t*)result.as.buffer)[0]); // 'H'
    TEST_ASSERT_EQUAL(101, ((uint8_t*)result.as.buffer)[1]); // 'e'
    vm_release(result);
}

// Test Buffer constructor error handling - invalid array elements
void test_buffer_class_constructor_error_handling(void) {
    // Test with valid values at boundaries
    value_t result = run_code("Buffer([0, 255, 128])");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_EQUAL(3, db_size(result.as.buffer));
    vm_release(result);
}

// ===========================
// BUFFER CLASS STATIC METHOD TESTS
// ===========================

// Test Buffer.fromHex() static method
void test_buffer_class_from_hex(void) {
    value_t result = run_code("Buffer.fromHex(\"48656c6c6f\")");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer));
    // Should represent "Hello"
    TEST_ASSERT_EQUAL(0, memcmp(result.as.buffer, "Hello", 5));
    vm_release(result);
}

// ===========================
// BUFFER CLASS INSTANCE METHOD TESTS
// ===========================

// Test Buffer instance method: length()
void test_buffer_class_length_method(void) {
    value_t result = run_code("Buffer(\"Hello World\").length()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(11, result.as.int32);
    vm_release(result);
}

// Test Buffer instance method: slice()
void test_buffer_class_slice_method(void) {
    value_t result = run_code("Buffer(\"Hello World\").slice(6, 5).toString()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("World", result.as.string);
    vm_release(result);
}

// Test Buffer instance method: concat()
void test_buffer_class_concat_method(void) {
    value_t result = run_code("Buffer(\"Hello\").concat(Buffer(\" World\")).toString()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);
}

// Test Buffer instance method: toHex()
void test_buffer_class_to_hex_method(void) {
    value_t result = run_code("Buffer(\"Hello\").toHex()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("48656c6c6f", result.as.string);
    vm_release(result);
}

// Test Buffer instance method: toString()
void test_buffer_class_to_string_method(void) {
    value_t result = run_code("Buffer(\"Hello World\").toString()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello World", result.as.string);
    vm_release(result);
}

// Test Buffer instance method: equals()
void test_buffer_class_equals_method(void) {
    // Test equal buffers
    value_t result = run_code("Buffer(\"Hello\").equals(Buffer(\"Hello\"))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test different buffers
    result = run_code("Buffer(\"Hello\").equals(Buffer(\"World\"))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
}

// Test Buffer instance method: reader()
void test_buffer_class_reader_method(void) {
    value_t result = run_code("Buffer(\"Hello\").reader()");
    TEST_ASSERT_EQUAL(VAL_BUFFER_READER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.reader);
    vm_release(result);
}

// ===========================
// BUFFER CLASS METHOD CHAINING TESTS
// ===========================

// Test Buffer chained method calls
void test_buffer_class_method_chaining(void) {
    value_t result = run_code("Buffer(\"Hello World\").slice(0, 5).concat(Buffer(\"!\")).toString()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Hello!", result.as.string);
    vm_release(result);
}

// Test Buffer reader method with buffer reader methods
void test_buffer_class_reader_integration(void) {
    // Test getting a reader and reading bytes
    value_t result = run_code("reader_read_uint8(Buffer(\"H\").reader())");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(72, result.as.int32); // ASCII 'H'
    vm_release(result);
}

// Test Buffer reader with positioning
void test_buffer_class_reader_positioning(void) {
    // Test remaining bytes in reader
    value_t result = run_code("reader_remaining(Buffer(\"Hello\").reader())");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(5, result.as.int32);
    vm_release(result);
    
    // Test position
    result = run_code("reader_position(Buffer(\"Hello\").reader())");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(0, result.as.int32);
    vm_release(result);
}

// ===========================
// BUFFER CLASS COMPREHENSIVE TESTS
// ===========================

// Test Buffer class with method chaining and reader
void test_buffer_class_comprehensive(void) {
    // Create a buffer, slice it, convert to hex, create from hex, and read
    value_t result = run_code("reader_read_uint8(Buffer.fromHex(Buffer(\"Hello World\").slice(0, 5).toHex()).reader())");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL(72, result.as.int32); // ASCII 'H' from "Hello"
    vm_release(result);
}

// Test Buffer class consistency with functional API
void test_buffer_class_vs_functional_api(void) {
    // Compare Buffer class methods with functional equivalents
    value_t class_result = run_code("Buffer(\"Hello\").toHex()");
    value_t func_result = run_code("buffer_to_hex(buffer(\"Hello\"))");
    
    TEST_ASSERT_EQUAL(VAL_STRING, class_result.type);
    TEST_ASSERT_EQUAL(VAL_STRING, func_result.type);
    TEST_ASSERT_EQUAL_STRING(class_result.as.string, func_result.as.string);
    
    vm_release(class_result);
    vm_release(func_result);
}

// Test Suite Runner
void test_buffer_class_suite(void) {
    RUN_TEST(test_buffer_class_constructor_string);
    RUN_TEST(test_buffer_class_constructor_array);
    RUN_TEST(test_buffer_class_constructor_error_handling);
    RUN_TEST(test_buffer_class_from_hex);
    RUN_TEST(test_buffer_class_length_method);
    RUN_TEST(test_buffer_class_slice_method);
    RUN_TEST(test_buffer_class_concat_method);
    RUN_TEST(test_buffer_class_to_hex_method);
    RUN_TEST(test_buffer_class_to_string_method);
    RUN_TEST(test_buffer_class_equals_method);
    RUN_TEST(test_buffer_class_reader_method);
    RUN_TEST(test_buffer_class_method_chaining);
    RUN_TEST(test_buffer_class_reader_integration);
    RUN_TEST(test_buffer_class_reader_positioning);
    RUN_TEST(test_buffer_class_comprehensive);
    RUN_TEST(test_buffer_class_vs_functional_api);
}