#include <string.h>
#include "codegen.h"
#include "lexer.h" 
#include "parser.h"
#include "unity.h"
#include "vm.h"
#include "datetime.h"

// Forward declaration of helper function (defined in test_vm.c)
extern value_t run_code(const char* source);

// Test LocalDate creation functions
void test_localdate_creation(void) {
    value_t result;
    
    // Test LocalDate.of creation
    result = run_code("LocalDate.of(2024, 12, 25)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_date->day);
    vm_release(result);
    
    // Test LocalDate.now creation (should not crash)
    result = run_code("LocalDate.now()");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    vm_release(result);
}

// Test new LocalDate factory syntax
void test_localdate_factory_syntax(void) {
    value_t result;
    
    // Test LocalDate(year, month, day) creation
    result = run_code("LocalDate(2024, 12, 25)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(25, result.as.local_date->day);
    vm_release(result);
    
    // Test edge cases with new syntax
    result = run_code("LocalDate(2024, 2, 29)"); // Leap day
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(29, result.as.local_date->day);
    vm_release(result);
    
    // Test compatibility: both syntaxes should work the same
    value_t old_syntax = run_code("LocalDate.of(2025, 6, 15)");
    value_t new_syntax = run_code("LocalDate(2025, 6, 15)");
    
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, old_syntax.type);
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, new_syntax.type);
    TEST_ASSERT_EQUAL_INT32(old_syntax.as.local_date->year, new_syntax.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(old_syntax.as.local_date->month, new_syntax.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(old_syntax.as.local_date->day, new_syntax.as.local_date->day);
    
    vm_release(old_syntax);
    vm_release(new_syntax);
}

// Test LocalDate getter methods
void test_localdate_getters(void) {
    value_t result;
    
    // Test year() method
    result = run_code("LocalDate.of(2024, 3, 15).year()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.int32);
    vm_release(result);
    
    // Test month() method
    result = run_code("LocalDate.of(2024, 3, 15).month()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(3, result.as.int32);
    vm_release(result);
    
    // Test day() method
    result = run_code("LocalDate.of(2024, 3, 15).day()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(15, result.as.int32);
    vm_release(result);
    
    // Test dayOfWeek() method (March 15, 2024 is a Friday = 5)
    result = run_code("LocalDate.of(2024, 3, 15).dayOfWeek()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    TEST_ASSERT_EQUAL_INT32(5, result.as.int32);
    vm_release(result);
}

// Test LocalDate arithmetic methods
void test_localdate_arithmetic(void) {
    value_t result;
    
    // Test plusDays() method
    result = run_code("LocalDate.of(2024, 3, 15).plusDays(7)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(3, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(22, result.as.local_date->day);
    vm_release(result);
    
    // Test minusDays() method
    result = run_code("LocalDate.of(2024, 3, 15).minusDays(10)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(3, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(5, result.as.local_date->day);
    vm_release(result);
    
    // Test plusMonths() method
    result = run_code("LocalDate.of(2024, 3, 15).plusMonths(2)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(5, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_date->day);
    vm_release(result);
    
    // Test minusMonths() method
    result = run_code("LocalDate.of(2024, 5, 15).minusMonths(3)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_date->day);
    vm_release(result);
    
    // Test plusYears() method
    result = run_code("LocalDate.of(2024, 3, 15).plusYears(5)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2029, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(3, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_date->day);
    vm_release(result);
    
    // Test minusYears() method
    result = run_code("LocalDate.of(2024, 3, 15).minusYears(10)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2014, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(3, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_date->day);
    vm_release(result);
}

// Test smart month arithmetic edge cases
void test_localdate_month_edge_cases(void) {
    value_t result;
    
    // Test January 31 + 1 month = February 29 (2024 is leap year)
    result = run_code("LocalDate.of(2024, 1, 31).plusMonths(1)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(29, result.as.local_date->day); // Clamped to leap day
    vm_release(result);
    
    // Test January 31 + 1 month in non-leap year = February 28
    result = run_code("LocalDate.of(2023, 1, 31).plusMonths(1)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2023, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(28, result.as.local_date->day); // Clamped to non-leap February
    vm_release(result);
    
    // Test May 31 + 1 month = June 30 (June has 30 days)
    result = run_code("LocalDate.of(2024, 5, 31).plusMonths(1)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(6, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(30, result.as.local_date->day); // Clamped to June max
    vm_release(result);
}

// Test LocalDate comparison methods
void test_localdate_comparisons(void) {
    value_t result;
    
    // Test equals() method - same dates
    result = run_code("LocalDate.of(2024, 3, 15).equals(LocalDate.of(2024, 3, 15))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test equals() method - different dates
    result = run_code("LocalDate.of(2024, 3, 15).equals(LocalDate.of(2024, 3, 16))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isBefore() method - true case
    result = run_code("LocalDate.of(2024, 3, 14).isBefore(LocalDate.of(2024, 3, 15))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test isBefore() method - false case
    result = run_code("LocalDate.of(2024, 3, 15).isBefore(LocalDate.of(2024, 3, 14))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
    
    // Test isAfter() method - true case
    result = run_code("LocalDate.of(2024, 3, 16).isAfter(LocalDate.of(2024, 3, 15))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_TRUE(result.as.boolean);
    vm_release(result);
    
    // Test isAfter() method - false case
    result = run_code("LocalDate.of(2024, 3, 15).isAfter(LocalDate.of(2024, 3, 16))");
    TEST_ASSERT_EQUAL_INT(VAL_BOOLEAN, result.type);
    TEST_ASSERT_FALSE(result.as.boolean);
    vm_release(result);
}

// Test year boundary arithmetic
void test_localdate_year_boundaries(void) {
    value_t result;
    
    // Test crossing year boundary with days
    result = run_code("LocalDate.of(2023, 12, 28).plusDays(7)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(1, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(4, result.as.local_date->day);
    vm_release(result);
    
    // Test crossing year boundary backwards with days
    result = run_code("LocalDate.of(2024, 1, 5).minusDays(10)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2023, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(12, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(26, result.as.local_date->day);
    vm_release(result);
    
    // Test crossing year boundary with months
    result = run_code("LocalDate.of(2023, 10, 15).plusMonths(5)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2024, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(3, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(15, result.as.local_date->day);
    vm_release(result);
}

// Test type() function with LocalDate
void test_localdate_type_function(void) {
    value_t result;
    
    result = run_code("type(LocalDate.of(2024, 3, 15))");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_NOT_NULL(result.as.string);
    TEST_ASSERT_EQUAL_STRING("LocalDate", result.as.string);
    vm_release(result);
}

// Test LocalDate string representation
void test_localdate_string_representation(void) {
    value_t result;
    
    // Test basic date formatting
    result = run_code("\"\" + LocalDate.of(2024, 3, 15)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_NOT_NULL(result.as.string);
    TEST_ASSERT_EQUAL_STRING("2024-03-15", result.as.string);
    vm_release(result);
    
    // Test single digit month and day padding
    result = run_code("\"\" + LocalDate.of(2024, 1, 5)");
    TEST_ASSERT_EQUAL_INT(VAL_STRING, result.type);
    TEST_ASSERT_NOT_NULL(result.as.string);
    TEST_ASSERT_EQUAL_STRING("2024-01-05", result.as.string);
    vm_release(result);
}

// Test invalid date creation (should handle gracefully)
void test_localdate_invalid_dates(void) {
    value_t result;
    
    // Test invalid month (should clamp or handle gracefully)
    // Note: This tests implementation behavior - may need adjustment based on actual error handling
    result = run_code("LocalDate.of(2024, 13, 15)");
    // Implementation may return null, throw error, or clamp - adjust assertion as needed
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
    
    // Test invalid day (should clamp or handle gracefully)  
    result = run_code("LocalDate.of(2024, 2, 30)");
    // Implementation may return null, throw error, or clamp - adjust assertion as needed
    if (result.type != VAL_NULL) {
        vm_release(result);
    }
}

// Test leap year edge cases
void test_localdate_leap_year_cases(void) {
    value_t result;
    
    // Test leap year day of week calculation (Feb 29, 2024)
    result = run_code("LocalDate.of(2024, 2, 29).dayOfWeek()");
    TEST_ASSERT_EQUAL_INT(VAL_INT32, result.type);
    // February 29, 2024 is a Thursday = 4
    TEST_ASSERT_EQUAL_INT32(4, result.as.int32);
    vm_release(result);
    
    // Test leap year + 4 years = next leap year
    result = run_code("LocalDate.of(2024, 2, 29).plusYears(4)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2028, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(29, result.as.local_date->day);
    vm_release(result);
    
    // Test leap year + 1 year = February 28 (clamped)
    result = run_code("LocalDate.of(2024, 2, 29).plusYears(1)");
    TEST_ASSERT_EQUAL_INT(VAL_LOCAL_DATE, result.type);
    TEST_ASSERT_NOT_NULL(result.as.local_date);
    TEST_ASSERT_EQUAL_INT32(2025, result.as.local_date->year);
    TEST_ASSERT_EQUAL_INT32(2, result.as.local_date->month);
    TEST_ASSERT_EQUAL_INT32(28, result.as.local_date->day); // Clamped to non-leap February
    vm_release(result);
}

// LocalDate test suite
void test_class_localdate_suite(void) {
    RUN_TEST(test_localdate_creation);
    RUN_TEST(test_localdate_factory_syntax);
    RUN_TEST(test_localdate_getters);
    RUN_TEST(test_localdate_arithmetic);
    RUN_TEST(test_localdate_month_edge_cases);
    RUN_TEST(test_localdate_comparisons);
    RUN_TEST(test_localdate_year_boundaries);
    RUN_TEST(test_localdate_type_function);
    RUN_TEST(test_localdate_string_representation);
    RUN_TEST(test_localdate_invalid_dates);
    RUN_TEST(test_localdate_leap_year_cases);
}