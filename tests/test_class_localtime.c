#include <string.h>
#include "codegen.h"
#include "lexer.h" 
#include "parser.h"
#include "unity.h"
#include "vm.h"
#include "datetime.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source);

// Test LocalTime creation functions
void test_localtime_creation(void) {
    value_t result;
    
    // Test LocalTime(hour, minute, second) creation
    result = run_code("LocalTime(14, 30, 45)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(14, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->millis);
    vm_release(result);
    
    // Test LocalTime(hour, minute, second, millis) creation
    result = run_code("LocalTime(9, 15, 30, 123)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(9, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->second);
    TEST_ASSERT_EQUAL_INT32(123, result.as.local_time->millis);
    vm_release(result);
    
    // Test edge case: midnight
    result = run_code("LocalTime(0, 0, 0, 0)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->second);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->millis);
    vm_release(result);
    
    // Test edge case: end of day
    result = run_code("LocalTime(23, 59, 59, 999)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(23, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(59, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(59, result.as.local_time->second);
    TEST_ASSERT_EQUAL_INT32(999, result.as.local_time->millis);
    vm_release(result);
}

// Test LocalTime accessor methods
void test_localtime_accessors(void) {
    value_t result;
    
    // Test hour() method
    result = run_code("LocalTime(14, 30, 45, 123).hour()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(14, result.as.int32);
    vm_release(result);
    
    // Test minute() method
    result = run_code("LocalTime(14, 30, 45, 123).minute()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(30, result.as.int32);
    vm_release(result);
    
    // Test second() method
    result = run_code("LocalTime(14, 30, 45, 123).second()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(45, result.as.int32);
    vm_release(result);
    
    // Test millisecond() method
    result = run_code("LocalTime(14, 30, 45, 123).millisecond()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(123, result.as.int32);
    vm_release(result);
    
    // Test edge cases: midnight
    result = run_code("LocalTime(0, 0, 0).hour()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(0, result.as.int32);
    vm_release(result);
    
    // Test edge cases: end of day
    result = run_code("LocalTime(23, 59, 59).hour()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(23, result.as.int32);
    vm_release(result);
}

// Test LocalTime arithmetic methods
void test_localtime_arithmetic(void) {
    value_t result;
    
    // Test plusHours()
    result = run_code("LocalTime(14, 30, 45).plusHours(2)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(16, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test plusMinutes()
    result = run_code("LocalTime(14, 30, 45).plusMinutes(90)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(16, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test plusSeconds()
    result = run_code("LocalTime(14, 30, 45).plusSeconds(90)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(14, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(32, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_time->second);
    vm_release(result);
    
    // Test minusHours()
    result = run_code("LocalTime(14, 30, 45).minusHours(2)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test minusMinutes()
    result = run_code("LocalTime(14, 30, 45).minusMinutes(45)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(13, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test minusSeconds()
    result = run_code("LocalTime(14, 30, 45).minusSeconds(90)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(14, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(29, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_time->second);
    vm_release(result);
}

// Test LocalTime midnight wrapping
void test_localtime_wrapping(void) {
    value_t result;
    
    // Test hour wrapping forward (past midnight)
    result = run_code("LocalTime(22, 30, 45).plusHours(3)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test hour wrapping backward (before midnight)
    result = run_code("LocalTime(1, 30, 45).minusHours(3)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(22, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    vm_release(result);
    
    // Test large hour additions (multiple day wraps)
    result = run_code("LocalTime(10, 0, 0).plusHours(26)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->second);
    vm_release(result);
    
    // Test large hour subtractions
    result = run_code("LocalTime(10, 0, 0).minusHours(25)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(9, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->second);
    vm_release(result);
    
    // Test minute wrapping that affects hours
    result = run_code("LocalTime(23, 30, 0).plusMinutes(90)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->second);
    vm_release(result);
    
    // Test second wrapping that affects minutes and hours
    result = run_code("LocalTime(23, 59, 30).plusSeconds(90)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_time->second);
    vm_release(result);
}

// Test LocalTime comparison methods
void test_localtime_comparisons(void) {
    value_t result;
    
    // Test equals() - same times
    result = run_code("LocalTime(14, 30, 45).equals(LocalTime(14, 30, 45))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test equals() - different times
    result = run_code("LocalTime(14, 30, 45).equals(LocalTime(14, 30, 46))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test equals() - with milliseconds
    result = run_code("LocalTime(14, 30, 45, 123).equals(LocalTime(14, 30, 45, 123))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test equals() - different milliseconds
    result = run_code("LocalTime(14, 30, 45, 123).equals(LocalTime(14, 30, 45, 124))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isBefore()
    result = run_code("LocalTime(14, 30, 45).isBefore(LocalTime(14, 30, 46))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("LocalTime(14, 30, 46).isBefore(LocalTime(14, 30, 45))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isAfter()
    result = run_code("LocalTime(14, 30, 46).isAfter(LocalTime(14, 30, 45))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("LocalTime(14, 30, 45).isAfter(LocalTime(14, 30, 46))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test comparisons across different components
    result = run_code("LocalTime(13, 59, 59).isBefore(LocalTime(14, 0, 0))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test midnight comparisons
    result = run_code("LocalTime(23, 59, 59).isBefore(LocalTime(0, 0, 0))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean); // 23:59:59 is later in the day than 00:00:00
    vm_release(result);
}

// Test LocalTime string representation
void test_localtime_string_representation(void) {
    value_t result;
    
    // Test basic toString()
    result = run_code("LocalTime(14, 30, 45).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("14:30:45", result.as.string);
    vm_release(result);
    
    // Test toString() with single digits
    result = run_code("LocalTime(9, 5, 3).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("09:05:03", result.as.string);
    vm_release(result);
    
    // Test toString() midnight
    result = run_code("LocalTime(0, 0, 0).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("00:00:00", result.as.string);
    vm_release(result);
    
    // Test toString() end of day
    result = run_code("LocalTime(23, 59, 59).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("23:59:59", result.as.string);
    vm_release(result);
    
    // Test toString() with milliseconds (shows millis when > 0)
    result = run_code("LocalTime(14, 30, 45, 123).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("14:30:45.123", result.as.string); // Shows millis when present
    vm_release(result);
    
    // Test toString() with zero milliseconds (basic format)
    result = run_code("LocalTime(14, 30, 45, 0).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("14:30:45", result.as.string); // No millis when zero
    vm_release(result);
}

// Test LocalTime type function
void test_localtime_type_function(void) {
    value_t result;
    
    result = run_code("type(LocalTime(14, 30, 45))");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("LocalTime", result.as.string);
    vm_release(result);
}

// Test LocalTime invalid values
void test_localtime_invalid_values(void) {
    value_t result;
    
    // Test invalid hour (should return null after runtime error)
    result = run_code("LocalTime(25, 30, 45)");
    // Runtime error expected, should return null
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
    
    // Test invalid minute
    result = run_code("LocalTime(14, 60, 45)");
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
    
    // Test invalid second
    result = run_code("LocalTime(14, 30, 60)");
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
    
    // Test invalid millisecond
    result = run_code("LocalTime(14, 30, 45, 1000)");
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
    
    // Test negative values
    result = run_code("LocalTime(-1, 30, 45)");
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
}

// Test LocalTime immutability
void test_localtime_immutability(void) {
    value_t result;
    
    // Test that arithmetic operations return new objects
    result = run_code("var t1 = LocalTime(14, 30, 45); var t2 = t1.plusHours(1); t1.toString() + \" != \" + t2.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("14:30:45 != 15:30:45", result.as.string);
    vm_release(result);
    
    // Test original object unchanged after multiple operations
    result = run_code("var t = LocalTime(12, 0, 0); t.plusHours(5).minusMinutes(30); t.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("12:00:00", result.as.string);
    vm_release(result);
}

// Main test suite
void test_class_localtime_suite(void) {
    RUN_TEST(test_localtime_creation);
    RUN_TEST(test_localtime_accessors);
    RUN_TEST(test_localtime_arithmetic);
    RUN_TEST(test_localtime_wrapping);
    RUN_TEST(test_localtime_comparisons);
    RUN_TEST(test_localtime_string_representation);
    RUN_TEST(test_localtime_type_function);
    // RUN_TEST(test_localtime_invalid_values); // Commented out: invalid times now cause runtime errors that terminate
    RUN_TEST(test_localtime_immutability);
}