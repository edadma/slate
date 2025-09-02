#include <string.h>
#include <time.h>
#include "codegen.h"
#include "lexer.h" 
#include "parser.h"
#include "unity.h"
#include "vm.h"
#include "datetime.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source);

// Helper function to compare bigint with int64
static bool di_equals_int64(di_int bigint_val, int64_t int64_val) {
    di_int expected = di_from_int64(int64_val);
    bool result = di_eq(bigint_val, expected);
    di_release(&expected);
    return result;
}

// Test Instant creation from milliseconds
void test_instant_creation_from_millis(void) {
    value_t result;
    
    // Test Instant(0) - Unix epoch
    result = run_code("Instant(0)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(0, result.as.instant_millis);
    vm_release(result);
    
    // Test Instant with positive milliseconds
    result = run_code("Instant(1609459200000)"); // January 1, 2021 00:00:00 UTC
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1609459200000LL, result.as.instant_millis);
    vm_release(result);
    
    // Test Instant with negative milliseconds (before epoch)
    result = run_code("Instant(-86400000)"); // December 31, 1969 00:00:00 UTC
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(-86400000LL, result.as.instant_millis);
    vm_release(result);
}

// Test Instant factory methods
void test_instant_factory_methods(void) {
    value_t result;
    
    // Test Instant.now() (should not crash and return reasonable value)
    result = run_code("Instant.now()");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    // Should be roughly current time (after 2020, before 2030)
    TEST_ASSERT_TRUE(result.as.instant_millis > 1577836800000LL); // After Jan 1, 2020
    TEST_ASSERT_TRUE(result.as.instant_millis < 1893456000000LL); // Before Jan 1, 2030
    vm_release(result);
    
    // Test Instant.ofEpochSecond()
    result = run_code("Instant.ofEpochSecond(1609459200)"); // January 1, 2021 00:00:00 UTC
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1609459200000LL, result.as.instant_millis);
    vm_release(result);
    
    // Test Instant.ofEpochSecond(0) - Unix epoch
    result = run_code("Instant.ofEpochSecond(0)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(0, result.as.instant_millis);
    vm_release(result);
}

// Test Instant string parsing
void test_instant_string_parsing(void) {
    value_t result;
    
    // Test ISO 8601 string parsing with seconds precision
    result = run_code("Instant.parse('2021-01-01T00:00:00Z')");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1609459200000LL, result.as.instant_millis);
    vm_release(result);
    
    // Test ISO 8601 string parsing with milliseconds
    result = run_code("Instant.parse('2021-01-01T00:00:00.500Z')");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1609459200500LL, result.as.instant_millis);
    vm_release(result);
    
    // Test factory constructor with string
    result = run_code("Instant('2021-01-01T00:00:00Z')");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1609459200000LL, result.as.instant_millis);
    vm_release(result);
}

// Test Instant conversion methods
void test_instant_conversion_methods(void) {
    value_t result;
    
    // Test toEpochMilli() - returns BigInt for large values
    result = run_code("val instant = Instant(1609459200500); instant.toEpochMilli()");
    TEST_ASSERT_EQUAL_INT(VAL_BIGINT, result.type);
    // For bigint, we can check if it matches expected value by converting
    TEST_ASSERT_TRUE(di_equals_int64(result.as.bigint, 1609459200500LL));
    vm_release(result);
    
    // Test toEpochSecond() - should truncate milliseconds
    result = run_code("val instant = Instant(1609459200500); instant.toEpochSecond()");
    TEST_ASSERT_EQUAL_INT(VAL_BIGINT, result.type);
    TEST_ASSERT_TRUE(di_equals_int64(result.as.bigint, 1609459200LL));
    vm_release(result);
    
    // Test toEpochSecond() with negative timestamp
    result = run_code("val instant = Instant(-1500); instant.toEpochSecond()");
    TEST_ASSERT_EQUAL_INT(VAL_BIGINT, result.type);
    TEST_ASSERT_TRUE(di_equals_int64(result.as.bigint, -2LL)); // -1500ms = -2 seconds (truncated)
    vm_release(result);
}

// Test Instant arithmetic operations
void test_instant_arithmetic_operations(void) {
    value_t result;
    
    // Test plusMillis()
    result = run_code("val instant = Instant(1000); instant.plusMillis(500)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(1500, result.as.instant_millis);
    vm_release(result);
    
    // Test minusMillis()
    result = run_code("val instant = Instant(1000); instant.minusMillis(500)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(500, result.as.instant_millis);
    vm_release(result);
    
    // Test plusSeconds()
    result = run_code("val instant = Instant(1000); instant.plusSeconds(2)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(3000, result.as.instant_millis);
    vm_release(result);
    
    // Test minusSeconds()
    result = run_code("val instant = Instant(5000); instant.minusSeconds(2)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(3000, result.as.instant_millis);
    vm_release(result);
    
    // Test method chaining
    result = run_code("val instant = Instant(1000); instant.plusSeconds(1).plusMillis(500)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(2500, result.as.instant_millis);
    vm_release(result);
}

// Test Instant comparison operations
void test_instant_comparison_operations(void) {
    value_t result;
    
    // Test isBefore()
    result = run_code("val a = Instant(1000); val b = Instant(2000); a.isBefore(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("val a = Instant(2000); val b = Instant(1000); a.isBefore(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isAfter()
    result = run_code("val a = Instant(2000); val b = Instant(1000); a.isAfter(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("val a = Instant(1000); val b = Instant(2000); a.isAfter(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test equals()
    result = run_code("val a = Instant(1000); val b = Instant(1000); a.equals(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("val a = Instant(1000); val b = Instant(2000); a.equals(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test equals() with different types
    result = run_code("val a = Instant(1000); val b = 1000; a.equals(b)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
}

// Test Instant string representation
void test_instant_string_representation(void) {
    value_t result;
    
    // Test toString() for epoch
    result = run_code("Instant(0).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00Z", result.as.string);
    vm_release(result);
    
    // Test toString() with milliseconds
    result = run_code("Instant(500).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00.500Z", result.as.string);
    vm_release(result);
    
    // Test toString() for specific date
    result = run_code("Instant(1609459200000).toString()"); // January 1, 2021 00:00:00 UTC
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2021-01-01T00:00:00Z", result.as.string);
    vm_release(result);
    
    // Test toString() for date with milliseconds
    result = run_code("Instant(1609459200750).toString()"); // January 1, 2021 00:00:00.750 UTC
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2021-01-01T00:00:00.750Z", result.as.string);
    vm_release(result);
}

// Test Instant edge cases and error handling
void test_instant_edge_cases(void) {
    value_t result;
    
    // Test very large positive timestamp (should work)
    result = run_code("Instant(253402300799999)"); // December 31, 9999 23:59:59.999 UTC
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(253402300799999LL, result.as.instant_millis);
    vm_release(result);
    
    // Test very large negative timestamp (should work)
    result = run_code("Instant(-62167219200000)"); // January 1, 0001 00:00:00 UTC
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(-62167219200000LL, result.as.instant_millis);
    vm_release(result);
    
    // Test arithmetic overflow protection in plusSeconds
    result = run_code("val instant = Instant(9223372036854775000); instant.plusSeconds(1000)");
    TEST_ASSERT_EQUAL_INT(VAL_NULL, result.type); // Should return null due to overflow error
    vm_release(result);
}

// Test Instant method chaining and fluent API
void test_instant_method_chaining(void) {
    value_t result;
    
    // Complex method chaining
    result = run_code("Instant(0).plusSeconds(60).plusMillis(500).minusSeconds(30)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_EQUAL_INT64(30500, result.as.instant_millis); // 30.5 seconds
    vm_release(result);
    
    // Chaining with comparison
    result = run_code("val base = Instant(1000); val derived = base.plusSeconds(5); derived.isAfter(base)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Chaining with string conversion
    result = run_code("Instant(0).plusMillis(123).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("1970-01-01T00:00:00.123Z", result.as.string);
    vm_release(result);
}

// Test Instant type checking and integration
void test_instant_type_system(void) {
    value_t result;
    
    // Test type() builtin
    result = run_code("type(Instant(0))");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("Instant", result.as.string);
    vm_release(result);
    
    // Test that Instant instances have the correct class
    result = run_code("Instant(0)");
    TEST_ASSERT_EQUAL_INT(VAL_INSTANT, result.type);
    TEST_ASSERT_NOT_NULL(result.class);
    vm_release(result);
}

// Instant test suite
void test_class_instant_suite(void) {
    RUN_TEST(test_instant_creation_from_millis);
    RUN_TEST(test_instant_factory_methods);
    RUN_TEST(test_instant_string_parsing);
    RUN_TEST(test_instant_conversion_methods);
    RUN_TEST(test_instant_arithmetic_operations);
    RUN_TEST(test_instant_comparison_operations);
    RUN_TEST(test_instant_string_representation);
    RUN_TEST(test_instant_edge_cases);
    RUN_TEST(test_instant_method_chaining);
    RUN_TEST(test_instant_type_system);
}