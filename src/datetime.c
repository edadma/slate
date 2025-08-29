#include "datetime.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global class references
value_t* global_local_date_class = NULL;
value_t* global_local_time_class = NULL;
value_t* global_local_datetime_class = NULL;
value_t* global_zoned_datetime_class = NULL;
value_t* global_instant_class = NULL;
value_t* global_duration_class = NULL;
value_t* global_period_class = NULL;

// Date constants
static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int DAYS_BEFORE_MONTH[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

// ============================================================================
// Date/Time validation functions
// ============================================================================

bool is_leap_year(int year) {
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int days_in_month(int year, int month) {
    if (month < 1 || month > 12) return 0;
    if (month == 2 && is_leap_year(year)) return 29;
    return DAYS_IN_MONTH[month - 1];
}

int days_in_year(int year) {
    return is_leap_year(year) ? 366 : 365;
}

bool is_valid_date(int year, int month, int day) {
    if (year < 1 || year > 9999) return false;
    if (month < 1 || month > 12) return false;
    if (day < 1 || day > days_in_month(year, month)) return false;
    return true;
}

bool is_valid_time(int hour, int minute, int second, int millis) {
    return hour >= 0 && hour <= 23 &&
           minute >= 0 && minute <= 59 &&
           second >= 0 && second <= 59 &&
           millis >= 0 && millis <= 999;
}

// ============================================================================
// Date arithmetic helpers
// ============================================================================

uint32_t date_to_epoch_day(int year, int month, int day) {
    // Algorithm based on Howard Hinnant's date library
    // Handles years 1-9999 correctly
    
    int y = year;
    int m = month;
    
    if (m <= 2) {
        y -= 1;
        m += 12;
    }
    
    // Calculate total days
    int era = y / 400;
    int yoe = y - era * 400;  // year of era
    int doy = (153 * (m - 3) + 2) / 5 + day - 1;  // day of year
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;  // day of era
    
    // Days since Unix epoch (1970-01-01)
    return era * 146097 + doe - 719468;
}

void epoch_day_to_date(uint32_t epoch_day, int* year, int* month, int* day) {
    // Reverse of date_to_epoch_day
    int z = epoch_day + 719468;
    int era = z / 146097;
    int doe = z - era * 146097;
    int yoe = (doe - doe/1460 + doe/36524 - doe/146096) / 365;
    int y = yoe + era * 400;
    int doy = doe - (365*yoe + yoe/4 - yoe/100);
    int mp = (5*doy + 2) / 153;
    int d = doy - (153*mp + 2) / 5 + 1;
    int m = mp + (mp < 10 ? 3 : -9);
    
    if (m <= 2) {
        y += 1;
    }
    
    *year = y;
    *month = m;
    *day = d;
}

// ============================================================================
// Local Date implementation
// ============================================================================

local_date_t* local_date_create(int year, int month, int day) {
    if (!is_valid_date(year, month, day)) {
        return NULL;
    }
    
    local_date_t* date = malloc(sizeof(local_date_t));
    if (!date) return NULL;
    
    date->ref_count = 1;
    date->year = year;
    date->month = month;
    date->day = day;
    date->epoch_day = date_to_epoch_day(year, month, day);
    
    return date;
}

local_date_t* local_date_now(void) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    
    return local_date_create(
        tm->tm_year + 1900,
        tm->tm_mon + 1,
        tm->tm_mday
    );
}

local_date_t* local_date_of_epoch_day(uint32_t epoch_day) {
    int year, month, day;
    epoch_day_to_date(epoch_day, &year, &month, &day);
    return local_date_create(year, month, day);
}

// ============================================================================
// Local Time implementation
// ============================================================================

local_time_t* local_time_create(int hour, int minute, int second, int millis) {
    if (!is_valid_time(hour, minute, second, millis)) {
        return NULL;
    }
    
    local_time_t* time = malloc(sizeof(local_time_t));
    if (!time) return NULL;
    
    time->ref_count = 1;
    time->hour = hour;
    time->minute = minute;
    time->second = second;
    time->millis = millis;
    time->nanos = millis * 1000000;  // Convert millis to total nanos
    
    return time;
}

local_time_t* local_time_now(void) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    
    struct tm* tm = localtime(&ts.tv_sec);
    int millis = ts.tv_nsec / 1000000;
    
    return local_time_create(
        tm->tm_hour,
        tm->tm_min,
        tm->tm_sec,
        millis
    );
}

// ============================================================================
// Memory management functions
// ============================================================================

local_date_t* local_date_retain(local_date_t* date) {
    if (date) {
        date->ref_count++;
    }
    return date;
}

void local_date_release(local_date_t* date) {
    if (date && --date->ref_count == 0) {
        free(date);
    }
}

local_time_t* local_time_retain(local_time_t* time) {
    if (time) {
        time->ref_count++;
    }
    return time;
}

void local_time_release(local_time_t* time) {
    if (time && --time->ref_count == 0) {
        free(time);
    }
}

local_datetime_t* local_datetime_retain(local_datetime_t* dt) {
    if (dt) {
        dt->ref_count++;
    }
    return dt;
}

void local_datetime_release(local_datetime_t* dt) {
    if (dt && --dt->ref_count == 0) {
        local_date_release(dt->date);
        local_time_release(dt->time);
        free(dt);
    }
}

zoned_datetime_t* zoned_datetime_retain(zoned_datetime_t* zdt) {
    if (zdt) {
        zdt->ref_count++;
    }
    return zdt;
}

void zoned_datetime_release(zoned_datetime_t* zdt) {
    if (zdt && --zdt->ref_count == 0) {
        local_datetime_release(zdt->dt);
        free(zdt->zone_id);
        free(zdt);
    }
}

instant_t* instant_retain(instant_t* instant) {
    if (instant) {
        instant->ref_count++;
    }
    return instant;
}

void instant_release(instant_t* instant) {
    if (instant && --instant->ref_count == 0) {
        free(instant);
    }
}

duration_t* duration_retain(duration_t* duration) {
    if (duration) {
        duration->ref_count++;
    }
    return duration;
}

void duration_release(duration_t* duration) {
    if (duration && --duration->ref_count == 0) {
        free(duration);
    }
}

period_t* period_retain(period_t* period) {
    if (period) {
        period->ref_count++;
    }
    return period;
}

void period_release(period_t* period) {
    if (period && --period->ref_count == 0) {
        free(period);
    }
}

// ============================================================================
// Value factory functions
// ============================================================================

value_t make_local_date_value(int year, int month, int day) {
    local_date_t* date = local_date_create(year, month, day);
    if (!date) {
        return make_null();
    }
    
    value_t val = {0};
    val.type = VAL_LOCAL_DATE;
    val.as.local_date = date;
    val.class = global_local_date_class;
    val.debug = NULL;
    
    return val;
}

value_t make_local_time_value(int hour, int minute, int second, int millis) {
    local_time_t* time = local_time_create(hour, minute, second, millis);
    if (!time) {
        return make_null();
    }
    
    value_t val = {0};
    val.type = VAL_LOCAL_TIME;
    val.as.local_time = time;
    val.class = global_local_time_class;
    val.debug = NULL;
    
    return val;
}

// ============================================================================
// Comparison functions
// ============================================================================

int local_date_compare(const local_date_t* a, const local_date_t* b) {
    if (a->epoch_day < b->epoch_day) return -1;
    if (a->epoch_day > b->epoch_day) return 1;
    return 0;
}

bool local_date_equals(const local_date_t* a, const local_date_t* b) {
    return a->epoch_day == b->epoch_day;
}

bool local_date_is_before(const local_date_t* a, const local_date_t* b) {
    return a->epoch_day < b->epoch_day;
}

bool local_date_is_after(const local_date_t* a, const local_date_t* b) {
    return a->epoch_day > b->epoch_day;
}

int local_time_compare(const local_time_t* a, const local_time_t* b) {
    // Compare total nanoseconds for precision
    uint64_t a_nanos = (uint64_t)a->hour * 3600000000000ULL + 
                       (uint64_t)a->minute * 60000000000ULL +
                       (uint64_t)a->second * 1000000000ULL + a->nanos;
    uint64_t b_nanos = (uint64_t)b->hour * 3600000000000ULL + 
                       (uint64_t)b->minute * 60000000000ULL +
                       (uint64_t)b->second * 1000000000ULL + b->nanos;
    
    if (a_nanos < b_nanos) return -1;
    if (a_nanos > b_nanos) return 1;
    return 0;
}

bool local_time_equals(const local_time_t* a, const local_time_t* b) {
    return local_time_compare(a, b) == 0;
}

// ============================================================================
// Date accessor functions
// ============================================================================

int local_date_get_year(const local_date_t* date) {
    return date->year;
}

int local_date_get_month(const local_date_t* date) {
    return date->month;
}

int local_date_get_day(const local_date_t* date) {
    return date->day;
}

int local_date_get_day_of_week(const local_date_t* date) {
    // Monday = 1, Sunday = 7
    // Unix epoch (1970-01-01) was a Thursday (4)
    return ((date->epoch_day + 3) % 7) + 1;
}

int local_date_get_day_of_year(const local_date_t* date) {
    int day_of_year = DAYS_BEFORE_MONTH[date->month - 1] + date->day;
    if (date->month > 2 && is_leap_year(date->year)) {
        day_of_year++;
    }
    return day_of_year;
}

// ============================================================================
// Date arithmetic functions
// ============================================================================

local_date_t* local_date_plus_days(const local_date_t* date, int days) {
    return local_date_of_epoch_day(date->epoch_day + days);
}

local_date_t* local_date_plus_months(const local_date_t* date, int months) {
    int new_year = date->year;
    int new_month = date->month + months;
    
    // Handle month overflow/underflow
    while (new_month > 12) {
        new_month -= 12;
        new_year++;
    }
    while (new_month < 1) {
        new_month += 12;
        new_year--;
    }
    
    // Clamp day to valid range for new month
    int new_day = date->day;
    int max_day = days_in_month(new_year, new_month);
    if (new_day > max_day) {
        new_day = max_day;
    }
    
    return local_date_create(new_year, new_month, new_day);
}

local_date_t* local_date_plus_years(const local_date_t* date, int years) {
    return local_date_plus_months(date, years * 12);
}

// ============================================================================
// String conversion functions
// ============================================================================

char* local_date_to_string(const local_date_t* date) {
    char* str = malloc(16);  // "YYYY-MM-DD" + null terminator
    if (!str) return NULL;
    
    snprintf(str, 16, "%04d-%02d-%02d", date->year, date->month, date->day);
    return str;
}

char* local_time_to_string(const local_time_t* time) {
    char* str = malloc(16);  // "HH:MM:SS.mmm" + null terminator
    if (!str) return NULL;
    
    if (time->millis > 0) {
        snprintf(str, 16, "%02d:%02d:%02d.%03d", 
                time->hour, time->minute, time->second, time->millis);
    } else {
        snprintf(str, 16, "%02d:%02d:%02d", 
                time->hour, time->minute, time->second);
    }
    return str;
}

char* local_datetime_to_string(const local_datetime_t* dt) {
    if (!dt || !dt->date || !dt->time) return NULL;
    
    char* date_str = local_date_to_string(dt->date);
    char* time_str = local_time_to_string(dt->time);
    
    if (!date_str || !time_str) {
        free(date_str);
        free(time_str);
        return NULL;
    }
    
    char* result = malloc(strlen(date_str) + strlen(time_str) + 2); // +1 for 'T', +1 for null
    if (!result) {
        free(date_str);
        free(time_str);
        return NULL;
    }
    
    sprintf(result, "%sT%s", date_str, time_str);
    
    free(date_str);
    free(time_str);
    
    return result;
}

// ============================================================================
// Built-in functions for VM integration
// ============================================================================

value_t builtin_local_date_now(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        vm_runtime_error_with_debug(vm, "LocalDate.now() takes no arguments");
        return make_null();
    }
    
    local_date_t* date = local_date_now();
    if (!date) {
        vm_runtime_error_with_debug(vm, "Failed to get current date");
        return make_null();
    }
    
    value_t val = {0};
    val.type = VAL_LOCAL_DATE;
    val.as.local_date = date;
    val.class = global_local_date_class;
    val.debug = NULL;
    
    return val;
}

value_t builtin_local_date_of(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 3) {
        vm_runtime_error_with_debug(vm, "LocalDate.of() requires 3 arguments: year, month, day");
        return make_null();
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        vm_runtime_error_with_debug(vm, "LocalDate.of() arguments must be numbers");
        return make_null();
    }
    
    int year = value_to_int(args[0]);
    int month = value_to_int(args[1]);
    int day = value_to_int(args[2]);
    
    if (!is_valid_date(year, month, day)) {
        vm_runtime_error_with_debug(vm, "Invalid date");
        return make_null();
    }
    
    return make_local_date_value(year, month, day);
}

// ============================================================================
// Initialization
// ============================================================================

void init_datetime_classes(slate_vm* vm) {
    // TODO: Initialize date/time classes and add to global scope
    // This will be implemented when we add the class infrastructure
}