#include "unity.h"
#include "test_helpers.h"
#include "vm.h"
#include "builtins.h"
#include "datetime.h"
#include "timezone.h"
#include "date_class.h"
#include "zone.h"

// No test fixture needed - test_execute_expression() handles VM creation and cleanup

// Test Date.now() returns a Date object
void test_date_now(void) {
    value_t result = test_execute_expression("Date.now()");
    
    TEST_ASSERT_EQUAL(VAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.date);
    
    vm_release(result);
}

// Test Date.of() creates a Date with specified components
void test_date_of(void) {
    value_t result = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 45, Zone.utc())");
    
    TEST_ASSERT_EQUAL(VAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.date);
    
    // Verify the date components by converting back to local datetime
    local_datetime_t* local_dt = date_get_local_datetime(result.as.date);
    TEST_ASSERT_NOT_NULL(local_dt);
    TEST_ASSERT_EQUAL(2024, local_dt->date->year);
    TEST_ASSERT_EQUAL(12, local_dt->date->month);
    TEST_ASSERT_EQUAL(25, local_dt->date->day);
    TEST_ASSERT_EQUAL(15, local_dt->time->hour);
    TEST_ASSERT_EQUAL(30, local_dt->time->minute);
    TEST_ASSERT_EQUAL(45, local_dt->time->second);
    
    vm_release(result);
}

// Test Date.nowInZone() with different timezones
void test_date_now_in_zone(void) {
    // Test with UTC
    value_t result_utc = test_execute_expression("Date.nowInZone(Zone.utc())");
    TEST_ASSERT_EQUAL(VAL_DATE, result_utc.type);
    TEST_ASSERT_NOT_NULL(result_utc.as.date);
    
    // Test with Toronto timezone (if available in embedded mode)
    value_t result_toronto = test_execute_expression("Date.nowInZone(Zone.of(\"America/Toronto\"))");
    TEST_ASSERT_EQUAL(VAL_DATE, result_toronto.type);
    TEST_ASSERT_NOT_NULL(result_toronto.as.date);
    
    vm_release(result_utc);
    vm_release(result_toronto);
}

// Test Date.fromInstant() conversion
void test_date_from_instant(void) {
    value_t result = test_execute_expression("Date.fromInstant(Instant.ofEpochSecond(1735128000), Zone.utc())"); // 2024-12-25 12:00:00 UTC
    
    TEST_ASSERT_EQUAL(VAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.date);
    
    // Verify the converted date
    local_datetime_t* local_dt = date_get_local_datetime(result.as.date);
    TEST_ASSERT_EQUAL(2024, local_dt->date->year);
    TEST_ASSERT_EQUAL(12, local_dt->date->month);
    TEST_ASSERT_EQUAL(25, local_dt->date->day);
    TEST_ASSERT_EQUAL(12, local_dt->time->hour);
    TEST_ASSERT_EQUAL(0, local_dt->time->minute);
    TEST_ASSERT_EQUAL(0, local_dt->time->second);
    
    vm_release(result);
}

// Test Date timezone operations
void test_date_timezone_operations(void) {
    // Create a date in UTC
    value_t utc_date = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 0, Zone.utc())");
    
    // Test zone() method
    value_t zone_result = test_execute_expression("var d = Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()); d.zone().id()");
    TEST_ASSERT_EQUAL(VAL_STRING, zone_result.type);
    TEST_ASSERT_EQUAL_STRING("UTC", zone_result.as.string);
    
    // Test withZone() conversion
    value_t converted = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).withZone(Zone.of(\"America/Toronto\"))");
    TEST_ASSERT_EQUAL(VAL_DATE, converted.type);
    
    // The local time should change due to timezone conversion
    local_datetime_t* converted_local = date_get_local_datetime(converted.as.date);
    TEST_ASSERT_NOT_EQUAL(15, converted_local->time->hour); // Should be different due to timezone offset
    
    vm_release(utc_date);
    vm_release(zone_result);
    vm_release(converted);
}

// Test Date arithmetic operations
void test_date_arithmetic(void) {
    // Test plusHours
    value_t plus_hours = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).plusHours(2)");
    TEST_ASSERT_EQUAL(VAL_DATE, plus_hours.type);
    local_datetime_t* plus_hours_local = date_get_local_datetime(plus_hours.as.date);
    TEST_ASSERT_EQUAL(17, plus_hours_local->time->hour);
    
    // Test plusDays
    value_t plus_days = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).plusDays(1)");
    TEST_ASSERT_EQUAL(VAL_DATE, plus_days.type);
    local_datetime_t* plus_days_local = date_get_local_datetime(plus_days.as.date);
    TEST_ASSERT_EQUAL(26, plus_days_local->date->day);
    
    // Test plusMonths
    value_t plus_months = test_execute_expression("Date.of(2024, 11, 25, 15, 30, 0, Zone.utc()).plusMonths(1)");
    TEST_ASSERT_EQUAL(VAL_DATE, plus_months.type);
    local_datetime_t* plus_months_local = date_get_local_datetime(plus_months.as.date);
    TEST_ASSERT_EQUAL(12, plus_months_local->date->month);
    
    vm_release(plus_hours);
    vm_release(plus_days);
    vm_release(plus_months);
}

// Test Date comparison operations
void test_date_comparisons(void) {
    // Test equals
    value_t equals_result = test_execute_expression(
        "Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).equals(Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, equals_result.type);
    TEST_ASSERT_TRUE(equals_result.as.boolean);
    
    // Test isBefore
    value_t before_result = test_execute_expression(
        "Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).isBefore(Date.of(2024, 12, 25, 16, 30, 0, Zone.utc()))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, before_result.type);
    TEST_ASSERT_TRUE(before_result.as.boolean);
    
    // Test isAfter
    value_t after_result = test_execute_expression(
        "Date.of(2024, 12, 25, 16, 30, 0, Zone.utc()).isAfter(Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()))");
    TEST_ASSERT_EQUAL(VAL_BOOLEAN, after_result.type);
    TEST_ASSERT_TRUE(after_result.as.boolean);
    
    vm_release(equals_result);
    vm_release(before_result);
    vm_release(after_result);
}

// Test Date toInstant() conversion
void test_date_to_instant(void) {
    value_t instant_result = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 0, Zone.utc()).toInstant()");
    
    TEST_ASSERT_EQUAL(VAL_INSTANT, instant_result.type);
    // The epoch milliseconds should be non-zero
    TEST_ASSERT_NOT_EQUAL(0, instant_result.as.instant_millis);
    
    vm_release(instant_result);
}

// Test Date toString() formatting
void test_date_to_string(void) {
    value_t string_result = test_execute_expression("Date.of(2024, 12, 25, 15, 30, 45, Zone.utc()).toString()");
    
    TEST_ASSERT_EQUAL(VAL_STRING, string_result.type);
    // Should be ISO 8601 format with timezone
    TEST_ASSERT_TRUE(strstr(string_result.as.string, "2024-12-25T15:30:45") != NULL);
    TEST_ASSERT_TRUE(strstr(string_result.as.string, "+00:00") != NULL);
    TEST_ASSERT_TRUE(strstr(string_result.as.string, "[UTC]") != NULL);
    
    vm_release(string_result);
}

// Test error handling for invalid arguments
void test_date_error_handling(void) {
    // Test Date.of() with wrong number of arguments
    value_t error_result1 = test_execute_expression("Date.of(2024, 12, 25)");
    TEST_ASSERT_EQUAL(VAL_NULL, error_result1.type);
    
    // Test Date.of() with invalid date
    value_t error_result2 = test_execute_expression("Date.of(2024, 13, 32, 25, 70, 80, Zone.utc())");
    TEST_ASSERT_EQUAL(VAL_NULL, error_result2.type);
    
    // Test Date.fromInstant() with wrong types
    value_t error_result3 = test_execute_expression("Date.fromInstant(\"not an instant\", Zone.utc())");
    TEST_ASSERT_EQUAL(VAL_NULL, error_result3.type);
    
    vm_release(error_result1);
    vm_release(error_result2);
    vm_release(error_result3);
}

// Test DST handling (if in embedded timezone mode with Canadian zones)
void test_date_dst_handling(void) {
    // Test creating a date during DST transition
    // March 10, 2024 is when DST starts in North America (2nd Sunday of March)
    value_t dst_date = test_execute_expression("Date.of(2024, 7, 15, 15, 30, 0, Zone.of(\"America/Toronto\"))");
    TEST_ASSERT_EQUAL(VAL_DATE, dst_date.type);
    
    // The timezone should be in DST during summer
    // (This test may vary depending on build configuration)
    
    vm_release(dst_date);
}

// Test memory management and object lifecycle
void test_date_memory_management(void) {
    // Create multiple Date objects and ensure they don't leak
    for (int i = 0; i < 100; i++) {
        value_t date = test_execute_expression("Date.now()");
        TEST_ASSERT_EQUAL(VAL_DATE, date.type);
        vm_release(date);
    }
    
    // Test that Date objects can be retained and released properly
    value_t date = test_execute_expression("Date.now()");
    value_t retained = vm_retain(date);
    TEST_ASSERT_EQUAL(date.as.date, retained.as.date);
    
    vm_release(date);
    vm_release(retained);
}

// Test suite function to run all Date class tests
void test_class_date_suite(void) {
    RUN_TEST(test_date_now);
    RUN_TEST(test_date_of);
    RUN_TEST(test_date_now_in_zone);
    RUN_TEST(test_date_from_instant);
    RUN_TEST(test_date_timezone_operations);
    RUN_TEST(test_date_arithmetic);
    RUN_TEST(test_date_comparisons);
    RUN_TEST(test_date_to_instant);
    RUN_TEST(test_date_to_string);
    RUN_TEST(test_date_error_handling);
    RUN_TEST(test_date_dst_handling);
    RUN_TEST(test_date_memory_management);
}