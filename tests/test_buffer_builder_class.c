#include "../unity/unity.h"
#include "test_helpers.h"
#include <string.h>

// ===========================
// BUFFER BUILDER CLASS CONSTRUCTOR TESTS
// ===========================

// Test BufferBuilder class constructor with valid capacity
void test_buffer_builder_class_constructor_valid(void) {
    value_t result = test_execute_expression("BufferBuilder(100)");
    TEST_ASSERT_EQUAL(VAL_BUFFER_BUILDER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.builder);
    // Verify it's a BufferBuilder object with methods
    TEST_ASSERT_NOT_NULL(result.class);
    vm_release(result);
}

// Test BufferBuilder constructor with zero capacity
void test_buffer_builder_class_constructor_zero_capacity(void) {
    value_t result = test_execute_expression("BufferBuilder(0)");
    TEST_ASSERT_EQUAL(VAL_BUFFER_BUILDER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.builder);
    vm_release(result);
}

// Test BufferBuilder constructor error handling - negative capacity
void test_buffer_builder_class_constructor_error_handling(void) {
    // This should fail with negative capacity
    bool error_occurred = test_expect_error("BufferBuilder(-1)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test BufferBuilder constructor error handling - wrong argument type
void test_buffer_builder_class_constructor_wrong_type(void) {
    // This should fail with string argument
    bool error_occurred = test_expect_error("BufferBuilder(\"hello\")", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// ===========================
// BUFFER BUILDER INSTANCE METHOD TESTS
// ===========================

// Test appendUint8 method
void test_buffer_builder_append_uint8(void) {
    value_t result = test_execute_expression("BufferBuilder(10).appendUint8(42).build()");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(1, db_size(result.as.buffer));
    TEST_ASSERT_EQUAL(42, ((uint8_t*)result.as.buffer)[0]);
    vm_release(result);
}

// Test appendUint16LE method
void test_buffer_builder_append_uint16_le(void) {
    value_t result = test_execute_expression("BufferBuilder(10).appendUint16LE(0x1234).build()");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_EQUAL(2, db_size(result.as.buffer));
    uint8_t* data = (uint8_t*)result.as.buffer;
    TEST_ASSERT_EQUAL(0x34, data[0]); // Little-endian low byte first
    TEST_ASSERT_EQUAL(0x12, data[1]); // Little-endian high byte second
    vm_release(result);
}

// Test appendUint32LE method
void test_buffer_builder_append_uint32_le(void) {
    value_t result = test_execute_expression("BufferBuilder(10).appendUint32LE(0x12345678).build()");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_EQUAL(4, db_size(result.as.buffer));
    uint8_t* data = (uint8_t*)result.as.buffer;
    TEST_ASSERT_EQUAL(0x78, data[0]); // Little-endian lowest byte first
    TEST_ASSERT_EQUAL(0x56, data[1]);
    TEST_ASSERT_EQUAL(0x34, data[2]);
    TEST_ASSERT_EQUAL(0x12, data[3]); // Little-endian highest byte last
    vm_release(result);
}

// Test appendString method
void test_buffer_builder_append_string(void) {
    value_t result = test_execute_expression("BufferBuilder(10).appendString(\"Hello\").build()");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_EQUAL(5, db_size(result.as.buffer)); // "Hello" is 5 bytes
    uint8_t* data = (uint8_t*)result.as.buffer;
    TEST_ASSERT_EQUAL('H', data[0]);
    TEST_ASSERT_EQUAL('e', data[1]);
    TEST_ASSERT_EQUAL('l', data[2]);
    TEST_ASSERT_EQUAL('l', data[3]);
    TEST_ASSERT_EQUAL('o', data[4]);
    vm_release(result);
}

// Test build method returns buffer
void test_buffer_builder_build_method(void) {
    value_t result = test_execute_expression("BufferBuilder(5).build()");
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.buffer);
    TEST_ASSERT_EQUAL(0, db_size(result.as.buffer)); // Empty buffer
    vm_release(result);
}

// ===========================
// BUFFER BUILDER METHOD CHAINING TESTS
// ===========================

// Test method chaining with multiple appends
void test_buffer_builder_method_chaining_comprehensive(void) {
    const char* code = "BufferBuilder(20)"
                      ".appendUint8(0xFF)"
                      ".appendUint16LE(0x1234)"
                      ".appendString(\"Hi\")"
                      ".appendUint32LE(0x12345678)"
                      ".build()";
    
    value_t result = test_execute_expression(code);
    TEST_ASSERT_EQUAL(VAL_BUFFER, result.type);
    
    // Expected: 0xFF + 0x34,0x12 + "Hi" + 0x78,0x56,0x34,0x12 = 9 bytes
    TEST_ASSERT_EQUAL(9, db_size(result.as.buffer));
    
    uint8_t* data = (uint8_t*)result.as.buffer;
    TEST_ASSERT_EQUAL(0xFF, data[0]);        // appendUint8
    TEST_ASSERT_EQUAL(0x34, data[1]);        // appendUint16LE low
    TEST_ASSERT_EQUAL(0x12, data[2]);        // appendUint16LE high
    TEST_ASSERT_EQUAL('H', data[3]);         // appendString
    TEST_ASSERT_EQUAL('i', data[4]);         // appendString
    TEST_ASSERT_EQUAL(0x78, data[5]);        // appendUint32LE byte 0 (little-endian)
    TEST_ASSERT_EQUAL(0x56, data[6]);        // appendUint32LE byte 1
    TEST_ASSERT_EQUAL(0x34, data[7]);        // appendUint32LE byte 2
    TEST_ASSERT_EQUAL(0x12, data[8]);        // appendUint32LE byte 3
    
    vm_release(result);
}

// Test intermediate method chaining returns BufferBuilder
void test_buffer_builder_method_chaining_intermediate(void) {
    value_t result = test_execute_expression("BufferBuilder(10).appendUint8(42)");
    TEST_ASSERT_EQUAL(VAL_BUFFER_BUILDER, result.type);
    TEST_ASSERT_NOT_NULL(result.as.builder);
    vm_release(result);
}

// ===========================
// BUFFER BUILDER STANDARD METHOD TESTS  
// ===========================

// Test toString method
void test_buffer_builder_to_string(void) {
    value_t result = test_execute_expression("BufferBuilder(10).toString()");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("[BufferBuilder]", result.as.string);
    vm_release(result);
}

// Test type checking
void test_buffer_builder_type_checking(void) {
    value_t result = test_execute_expression("type(BufferBuilder(10))");
    TEST_ASSERT_EQUAL(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("buffer_builder", result.as.string);
    vm_release(result);
}

// Test hash method exists and returns integer
void test_buffer_builder_hash_method(void) {
    value_t result = test_execute_expression("BufferBuilder(10).hash()");
    TEST_ASSERT_EQUAL(VAL_INT32, result.type);
    // Hash should be some integer value (exact value doesn't matter for this test)
    vm_release(result);
}

// Test equals method
void test_buffer_builder_equals_method(void) {
    // Same BufferBuilder instance should equal itself
    value_t result = test_execute_expression("var b = BufferBuilder(10); b.equals(b)");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Different BufferBuilder instances should not be equal
    result = test_execute_expression("BufferBuilder(10).equals(BufferBuilder(10))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
}

// ===========================
// BUFFER BUILDER ERROR HANDLING TESTS
// ===========================

// Test appendUint8 bounds checking
void test_buffer_builder_append_uint8_bounds(void) {
    // Test negative value
    bool error_occurred = test_expect_error("BufferBuilder(10).appendUint8(-1)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
    
    // Test value too large  
    error_occurred = test_expect_error("BufferBuilder(10).appendUint8(256)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test appendUint16LE bounds checking
void test_buffer_builder_append_uint16_le_bounds(void) {
    // Test negative value
    bool error_occurred = test_expect_error("BufferBuilder(10).appendUint16LE(-1)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
    
    // Test value too large
    error_occurred = test_expect_error("BufferBuilder(10).appendUint16LE(65536)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// Test appendUint32LE bounds checking
void test_buffer_builder_append_uint32_le_bounds(void) {
    // Test negative value
    bool error_occurred = test_expect_error("BufferBuilder(10).appendUint32LE(-1)", ERR_TYPE);
    TEST_ASSERT_TRUE(error_occurred);
}

// ===========================
// BUFFER BUILDER TEST SUITE
// ===========================

void test_buffer_builder_class_suite(void) {
    RUN_TEST(test_buffer_builder_class_constructor_valid);
    RUN_TEST(test_buffer_builder_class_constructor_zero_capacity);
    RUN_TEST(test_buffer_builder_class_constructor_error_handling);
    RUN_TEST(test_buffer_builder_class_constructor_wrong_type);
    
    RUN_TEST(test_buffer_builder_append_uint8);
    RUN_TEST(test_buffer_builder_append_uint16_le);
    RUN_TEST(test_buffer_builder_append_uint32_le);
    RUN_TEST(test_buffer_builder_append_string);
    RUN_TEST(test_buffer_builder_build_method);
    
    RUN_TEST(test_buffer_builder_method_chaining_comprehensive);
    RUN_TEST(test_buffer_builder_method_chaining_intermediate);
    
    RUN_TEST(test_buffer_builder_to_string);
    RUN_TEST(test_buffer_builder_type_checking);
    RUN_TEST(test_buffer_builder_hash_method);
    RUN_TEST(test_buffer_builder_equals_method);
    
    RUN_TEST(test_buffer_builder_append_uint8_bounds);
    RUN_TEST(test_buffer_builder_append_uint16_le_bounds);
    RUN_TEST(test_buffer_builder_append_uint32_le_bounds);
}