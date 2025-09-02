#include <string.h>
#include "codegen.h"
#include "lexer.h" 
#include "parser.h"
#include "unity.h"
#include "vm.h"
#include "datetime.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source);

// Test LocalDateTime creation from ISO string
void test_localdatetime_iso_string_creation(void) {
    value_t result;
    
    // Test basic ISO string parsing
    result = run_code("LocalDateTime(\"2024-12-25T15:30:45\")");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_datetime->time->second);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->millis);
    vm_release(result);
    
    // Test ISO string with milliseconds
    result = run_code("LocalDateTime(\"2024-01-01T09:30:15.123\")");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(9, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_datetime->time->second);
    TEST_ASSERT_EQUAL_INT32(123, result.as.local_datetime->time->millis);
    vm_release(result);
    
    // Test ISO string with space separator
    result = run_code("LocalDateTime(\"2025-06-15 12:00:00\")");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2025, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(6, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->second);
    vm_release(result);
}

// Test LocalDateTime creation from components
void test_localdatetime_component_creation(void) {
    value_t result;
    
    // Test creation with year, month, day, hour, minute, second
    result = run_code("LocalDateTime(2024, 12, 25, 15, 30, 45)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_datetime->time->second);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->millis);
    vm_release(result);
    
    // Test creation with milliseconds
    result = run_code("LocalDateTime(2024, 1, 1, 0, 0, 0, 999)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(0, result.as.local_datetime->time->second);
    TEST_ASSERT_EQUAL_INT32(999, result.as.local_datetime->time->millis);
    vm_release(result);
}

// Test LocalDateTime creation from LocalDate and LocalTime
void test_localdatetime_date_time_creation(void) {
    value_t result;
    
    // Test creation from LocalDate and LocalTime objects
    result = run_code("var date = LocalDate.of(2024, 12, 25); var time = LocalTime(15, 30, 45); LocalDateTime(date, time)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATETIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_datetime);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_datetime->date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_datetime->date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_datetime->date->day);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_datetime->time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_datetime->time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_datetime->time->second);
    vm_release(result);
}

// Test LocalDateTime accessor methods
void test_localdatetime_accessors(void) {
    value_t result;
    
    // Create a LocalDateTime and test all accessor methods
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.year()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.month()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(12, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.day()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(25, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.hour()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.minute()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(30, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.second()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(45, result.as.int32);
    vm_release(result);
    
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.millisecond()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(123, result.as.int32);
    vm_release(result);
}

// Test LocalDateTime toString method
void test_localdatetime_to_string(void) {
    value_t result;
    
    // Test toString without milliseconds
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45\"); dt.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-25T15:30:45", result.as.string);
    vm_release(result);
    
    // Test toString with milliseconds
    result = run_code("var dt = LocalDateTime(\"2024-01-01T09:15:30.456\"); dt.toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-01-01T09:15:30.456", result.as.string);
    vm_release(result);
    
    // Test string conversion (implicit toString)
    result = run_code("var dt = LocalDateTime(2024, 6, 15, 12, 0, 0); \"\" + dt");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-06-15T12:00:00", result.as.string);
    vm_release(result);
}

// Test LocalDateTime arithmetic methods (plus operations)
void test_localdatetime_plus_operations(void) {
    value_t result;
    
    // Test plusYears
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45\"); dt.plusYears(1).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2025-12-25T15:30:45", result.as.string);
    vm_release(result);
    
    // Test plusMonths
    result = run_code("var dt = LocalDateTime(\"2024-01-31T12:00:00\"); dt.plusMonths(1).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    // Note: Should handle month-end overflow correctly
    vm_release(result);
    
    // Test plusDays
    result = run_code("var dt = LocalDateTime(\"2024-12-31T23:59:59\"); dt.plusDays(1).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2025-01-01T23:59:59", result.as.string);
    vm_release(result);
    
    // Test plusHours
    result = run_code("var dt = LocalDateTime(\"2024-12-25T23:30:45\"); dt.plusHours(2).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-26T01:30:45", result.as.string);
    vm_release(result);
    
    // Test plusMinutes
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:45:00\"); dt.plusMinutes(30).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-25T16:15:00", result.as.string);
    vm_release(result);
    
    // Test plusSeconds
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:30\"); dt.plusSeconds(45).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-25T15:31:15", result.as.string);
    vm_release(result);
    
    // Test minusSeconds (negative seconds operation)
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45\"); dt.minusSeconds(15).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-25T15:30:30", result.as.string);
    vm_release(result);
}

// Test LocalDateTime comparison methods
void test_localdatetime_comparisons(void) {
    value_t result;
    
    // Test isBefore
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T10:00:00\"); var dt2 = LocalDateTime(\"2024-12-25T15:00:00\"); dt1.isBefore(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T15:00:00\"); var dt2 = LocalDateTime(\"2024-12-25T10:00:00\"); dt1.isBefore(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isAfter
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T15:00:00\"); var dt2 = LocalDateTime(\"2024-12-25T10:00:00\"); dt1.isAfter(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T10:00:00\"); var dt2 = LocalDateTime(\"2024-12-25T15:00:00\"); dt1.isAfter(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test equals
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T15:30:45\"); var dt2 = LocalDateTime(\"2024-12-25T15:30:45\"); dt1.equals(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T15:30:45\"); var dt2 = LocalDateTime(\"2024-12-25T15:30:46\"); dt1.equals(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test millisecond precision in comparisons
    result = run_code("var dt1 = LocalDateTime(\"2024-12-25T15:30:45.123\"); var dt2 = LocalDateTime(\"2024-12-25T15:30:45.124\"); dt1.isBefore(dt2)");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
}

// Test LocalDateTime toDate and toTime methods
void test_localdatetime_conversion_methods(void) {
    value_t result;
    
    // Test date()
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45\"); dt.date()");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_date->day);
    vm_release(result);
    
    // Test time()
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45.123\"); dt.time()");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_TIME, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_time);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_time->hour);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_time->minute);
    TEST_ASSERT_EQUAL_INT32(45, result.as.local_time->second);
    TEST_ASSERT_EQUAL_INT32(123, result.as.local_time->millis);
    vm_release(result);
}

// Test LocalDateTime type checking
void test_localdatetime_type_checking(void) {
    value_t result;
    
    // Test type() function
    result = run_code("var dt = LocalDateTime(\"2024-12-25T15:30:45\"); type(dt)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("LocalDateTime", result.as.string);
    vm_release(result);
}

// Test LocalDateTime error cases
void test_localdatetime_error_cases(void) {
    // Note: These tests should verify that appropriate runtime errors are thrown
    // The run_code helper might need to be wrapped in error handling to test these properly
    
    // Invalid ISO string formats should throw errors
    // Invalid component values should throw errors  
    // Null parameters should throw errors
}

// Test LocalDateTime method chaining
void test_localdatetime_method_chaining(void) {
    value_t result;
    
    // Test chaining multiple operations
    result = run_code("LocalDateTime(\"2024-01-01T00:00:00\").plusYears(1).plusMonths(11).plusDays(24).plusHours(23).plusMinutes(59).plusSeconds(59).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2025-12-25T23:59:59", result.as.string);
    vm_release(result);
    
    // Test mixed operations
    result = run_code("LocalDateTime(2024, 6, 15, 12, 0, 0).plusDays(10).date().toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-06-25", result.as.string);
    vm_release(result);
}

// Test LocalDateTime edge cases
void test_localdatetime_edge_cases(void) {
    value_t result;
    
    // Test leap year handling
    result = run_code("LocalDateTime(\"2024-02-29T12:00:00\").toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-02-29T12:00:00", result.as.string);
    vm_release(result);
    
    // Test year boundaries
    result = run_code("LocalDateTime(\"2023-12-31T23:59:59\").plusSeconds(1).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-01-01T00:00:00", result.as.string);
    vm_release(result);
    
    // Test second overflow  
    result = run_code("LocalDateTime(\"2024-12-25T15:30:59\").plusSeconds(1).toString()");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_EQUAL_STRING("2024-12-25T15:31:00", result.as.string);
    vm_release(result);
}

// LocalDateTime test suite
void test_class_local_date_time_suite(void) {
    RUN_TEST(test_localdatetime_iso_string_creation);
    RUN_TEST(test_localdatetime_component_creation);
    RUN_TEST(test_localdatetime_date_time_creation);
    RUN_TEST(test_localdatetime_accessors);
    RUN_TEST(test_localdatetime_to_string);
    RUN_TEST(test_localdatetime_plus_operations);
    RUN_TEST(test_localdatetime_comparisons);
    RUN_TEST(test_localdatetime_conversion_methods);
    RUN_TEST(test_localdatetime_type_checking);
    RUN_TEST(test_localdatetime_error_cases);
    RUN_TEST(test_localdatetime_method_chaining);
    RUN_TEST(test_localdatetime_edge_cases);
}