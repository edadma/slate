#include "datetime.h"
#include "builtins.h"
#include "instant.h"
#include "zone.h"
#include "timezone.h"
#include "date_class.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Global class references
value_t* global_local_date_class = NULL;
value_t* global_local_time_class = NULL;
value_t* global_local_datetime_class = NULL;
value_t* global_date_class = NULL;
value_t* global_instant_class = NULL;
value_t* global_duration_class = NULL;
value_t* global_period_class = NULL;

// Date constants
static const int DAYS_IN_MONTH[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
static const int DAYS_BEFORE_MONTH[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};

// ============================================================================
// Date/Time validation functions
// ============================================================================

bool is_leap_year(int year) { return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0); }

int days_in_month(int year, int month) {
    if (month < 1 || month > 12)
        return 0;
    if (month == 2 && is_leap_year(year))
        return 29;
    return DAYS_IN_MONTH[month - 1];
}

int days_in_year(int year) { return is_leap_year(year) ? 366 : 365; }

bool is_valid_date(int year, int month, int day) {
    if (year < 1 || year > 9999)
        return false;
    if (month < 1 || month > 12)
        return false;
    if (day < 1 || day > days_in_month(year, month))
        return false;
    return true;
}

bool is_valid_time(int hour, int minute, int second, int millis) {
    return hour >= 0 && hour <= 23 && minute >= 0 && minute <= 59 && second >= 0 && second <= 59 && millis >= 0 &&
        millis <= 999;
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
    int yoe = y - era * 400; // year of era
    int doy = (153 * (m - 3) + 2) / 5 + day - 1; // day of year
    int doe = yoe * 365 + yoe / 4 - yoe / 100 + doy; // day of era

    // Days since Unix epoch (1970-01-01)
    return era * 146097 + doe - 719468;
}

void epoch_day_to_date(uint32_t epoch_day, int* year, int* month, int* day) {
    // Reverse of date_to_epoch_day
    int z = epoch_day + 719468;
    int era = z / 146097;
    int doe = z - era * 146097;
    int yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
    int y = yoe + era * 400;
    int doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
    int mp = (5 * doy + 2) / 153;
    int d = doy - (153 * mp + 2) / 5 + 1;
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

local_date_t* local_date_create(vm_t* vm, int year, int month, int day) {
    if (!is_valid_date(year, month, day)) {
        if (vm) {
            runtime_error(vm, "Invalid date parameters");
        }
        return NULL; // Fallback for NULL VM case
    }

    local_date_t* date = malloc(sizeof(local_date_t));
    if (date == NULL) {
        runtime_error(vm, "Memory allocation failed for LocalDate");
    }

    date->ref_count = 1;
    date->year = year;
    date->month = month;
    date->day = day;
    date->epoch_day = date_to_epoch_day(year, month, day);

    return date;
}

local_date_t* local_date_now(vm_t* vm) {
    time_t t = time(NULL);
    struct tm* tm = localtime(&t);
    if (tm == NULL) {
        runtime_error(vm, "System time function failed");
    }

    return local_date_create(vm, tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday);
}

local_date_t* local_date_of_epoch_day(vm_t* vm, uint32_t epoch_day) {
    int year, month, day;
    epoch_day_to_date(epoch_day, &year, &month, &day);
    return local_date_create(vm, year, month, day);
}

// ============================================================================
// Local Time implementation
// ============================================================================

local_time_t* local_time_create(vm_t* vm, int hour, int minute, int second, int millis) {
    if (!is_valid_time(hour, minute, second, millis)) {
        if (vm) {
            runtime_error(vm, "Invalid time parameters");
        }
        return NULL; // Fallback for NULL VM case
    }

    local_time_t* time = malloc(sizeof(local_time_t));
    if (time == NULL) {
        runtime_error(vm, "Memory allocation failed for LocalTime");
    }

    time->ref_count = 1;
    time->hour = hour;
    time->minute = minute;
    time->second = second;
    time->millis = millis;
    time->nanos = millis * 1000000; // Convert millis to total nanos

    return time;
}

local_time_t* local_time_now(vm_t* vm) {
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);

    struct tm* tm = localtime(&ts.tv_sec);
    if (tm == NULL) {
        runtime_error(vm, "System time function failed");
    }
    int millis = ts.tv_nsec / 1000000;

    return local_time_create(vm, tm->tm_hour, tm->tm_min, tm->tm_sec, millis);
}

// ============================================================================
// Local DateTime implementation
// ============================================================================

local_datetime_t* local_datetime_create(vm_t* vm, local_date_t* date, local_time_t* time) {
    if (date == NULL || time == NULL) {
        runtime_error(vm, "Invalid null date or time parameter");
    }
    
    local_datetime_t* dt = malloc(sizeof(local_datetime_t));
    if (dt == NULL) {
        runtime_error(vm, "Memory allocation failed for LocalDateTime");
    }
    
    dt->ref_count = 1;
    dt->date = local_date_retain(date);
    dt->time = local_time_retain(time);
    
    return dt;
}

local_datetime_t* local_datetime_now(vm_t* vm) {
    local_date_t* date = local_date_now(vm);
    local_time_t* time = local_time_now(vm);
    
    local_datetime_t* dt = local_datetime_create(vm, date, time);
    
    // Release our references since local_datetime_create retains them
    local_date_release(date);
    local_time_release(time);
    
    return dt;
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

// Note: date_retain and date_release are now implemented in date.c
// These old zoned_datetime functions are kept for compatibility but deprecated

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

value_t make_local_date_value(vm_t* vm, int year, int month, int day) {
    local_date_t* date = local_date_create(vm, year, month, day);

    value_t val = {0};
    val.type = VAL_LOCAL_DATE;
    val.as.local_date = date;
    val.class = global_local_date_class;
    val.debug = NULL;

    return val;
}

value_t make_local_time_value(vm_t* vm, int hour, int minute, int second, int millis) {
    local_time_t* time = local_time_create(vm, hour, minute, second, millis);

    value_t val = {0};
    val.type = VAL_LOCAL_TIME;
    val.as.local_time = time;
    val.class = global_local_time_class;
    val.debug = NULL;

    return val;
}

value_t make_local_datetime_value(vm_t* vm, local_date_t* date, local_time_t* time) {
    local_datetime_t* dt = local_datetime_create(vm, date, time);

    value_t val = {0};
    val.type = VAL_LOCAL_DATETIME;
    val.as.local_datetime = dt;
    val.class = global_local_datetime_class;
    val.debug = NULL;

    return val;
}

// ============================================================================
// Comparison functions
// ============================================================================

int local_date_compare(const local_date_t* a, const local_date_t* b) {
    if (a->epoch_day < b->epoch_day)
        return -1;
    if (a->epoch_day > b->epoch_day)
        return 1;
    return 0;
}

bool local_date_equals(const local_date_t* a, const local_date_t* b) { return a->epoch_day == b->epoch_day; }

bool local_date_is_before(const local_date_t* a, const local_date_t* b) { return a->epoch_day < b->epoch_day; }

bool local_date_is_after(const local_date_t* a, const local_date_t* b) { return a->epoch_day > b->epoch_day; }

int local_time_compare(const local_time_t* a, const local_time_t* b) {
    // Compare total nanoseconds for precision
    uint64_t a_nanos = (uint64_t)a->hour * 3600000000000ULL + (uint64_t)a->minute * 60000000000ULL +
        (uint64_t)a->second * 1000000000ULL + a->nanos;
    uint64_t b_nanos = (uint64_t)b->hour * 3600000000000ULL + (uint64_t)b->minute * 60000000000ULL +
        (uint64_t)b->second * 1000000000ULL + b->nanos;

    if (a_nanos < b_nanos)
        return -1;
    if (a_nanos > b_nanos)
        return 1;
    return 0;
}

bool local_time_equals(const local_time_t* a, const local_time_t* b) { return local_time_compare(a, b) == 0; }

// ============================================================================
// Date accessor functions
// ============================================================================

int local_date_get_year(const local_date_t* date) { return date->year; }

int local_date_get_month(const local_date_t* date) { return date->month; }

int local_date_get_day(const local_date_t* date) { return date->day; }

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

local_date_t* local_date_plus_days(vm_t* vm, const local_date_t* date, int days) {
    if (date == NULL) {
        runtime_error(vm, "Invalid null date parameter");
    }
    return local_date_of_epoch_day(vm, date->epoch_day + days);
}

local_date_t* local_date_plus_months(vm_t* vm, const local_date_t* date, int months) {
    if (date == NULL) {
        runtime_error(vm, "Invalid null date parameter");
    }
    
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

    return local_date_create(vm, new_year, new_month, new_day);
}

local_date_t* local_date_plus_years(vm_t* vm, const local_date_t* date, int years) {
    return local_date_plus_months(vm, date, years * 12);
}

// ============================================================================
// String conversion functions
// ============================================================================

char* local_date_to_string(vm_t* vm, const local_date_t* date) {
    if (date == NULL) {
        if (vm) {
            runtime_error(vm, "Invalid null date parameter");
        }
        return NULL; // Fallback for NULL VM case
    }
    
    char* str = malloc(16); // "YYYY-MM-DD" + null terminator
    if (str == NULL) {
        if (vm) {
            runtime_error(vm, "Memory allocation failed for date string");
        }
        return NULL; // Fallback for NULL VM case
    }

    snprintf(str, 16, "%04d-%02d-%02d", date->year, date->month, date->day);
    return str;
}

char* local_time_to_string(vm_t* vm, const local_time_t* time) {
    if (time == NULL) {
        if (vm) {
            runtime_error(vm, "Invalid null time parameter");
        }
        return NULL; // Fallback for NULL VM case
    }
    
    char* str = malloc(18); // "HH:MM:SS.mmm" + null terminator
    if (str == NULL) {
        if (vm) {
            runtime_error(vm, "Memory allocation failed for time string");
        }
        return NULL; // Fallback for NULL VM case
    }

    if (time->millis > 0) {
        snprintf(str, 18, "%02d:%02d:%02d.%03d", time->hour, time->minute, time->second, time->millis);
    } else {
        snprintf(str, 18, "%02d:%02d:%02d", time->hour, time->minute, time->second);
    }
    return str;
}

char* local_datetime_to_string(vm_t* vm, const local_datetime_t* dt) {
    if (dt == NULL || dt->date == NULL || dt->time == NULL) {
        if (vm) {
            runtime_error(vm, "Invalid null datetime parameter");
        }
        return NULL; // Fallback for NULL VM case
    }

    char* date_str = local_date_to_string(vm, dt->date);
    char* time_str = local_time_to_string(vm, dt->time);

    char* result = malloc(strlen(date_str) + strlen(time_str) + 2); // +1 for 'T', +1 for null
    if (result == NULL) {
        free(date_str);
        free(time_str);
        if (vm) {
            runtime_error(vm, "Memory allocation failed for datetime string");
        }
        return NULL; // Fallback for NULL VM case
    }

    sprintf(result, "%sT%s", date_str, time_str);

    free(date_str);
    free(time_str);

    return result;
}

// ============================================================================
// Local Time accessor functions
// ============================================================================

int local_time_get_hour(const local_time_t* time) {
    if (time == NULL) {
        return -1; // Error sentinel value
    }
    return time->hour;
}

int local_time_get_minute(const local_time_t* time) {
    if (time == NULL) {
        return -1; // Error sentinel value
    }
    return time->minute;
}

int local_time_get_second(const local_time_t* time) {
    if (time == NULL) {
        return -1; // Error sentinel value
    }
    return time->second;
}

int local_time_get_millisecond(const local_time_t* time) {
    if (time == NULL) {
        return -1; // Error sentinel value
    }
    return time->millis;
}

// ============================================================================
// Local Time arithmetic functions
// ============================================================================

local_time_t* local_time_plus_hours(vm_t* vm, const local_time_t* time, int hours) {
    if (time == NULL) {
        runtime_error(vm, "Invalid null time parameter");
    }
    
    int total_hours = time->hour + hours;
    
    // Normalize hours to 0-23 range
    total_hours = total_hours % 24;
    if (total_hours < 0) {
        total_hours += 24;
    }
    
    return local_time_create(vm, total_hours, time->minute, time->second, time->millis);
}

local_time_t* local_time_plus_minutes(vm_t* vm, const local_time_t* time, int minutes) {
    if (time == NULL) {
        runtime_error(vm, "Invalid null time parameter");
    }
    
    int total_minutes = time->hour * 60 + time->minute + minutes;
    
    // Handle negative values
    while (total_minutes < 0) {
        total_minutes += 24 * 60;
    }
    
    int new_hour = (total_minutes / 60) % 24;
    int new_minute = total_minutes % 60;
    
    return local_time_create(vm, new_hour, new_minute, time->second, time->millis);
}

local_time_t* local_time_plus_seconds(vm_t* vm, const local_time_t* time, int seconds) {
    if (time == NULL) {
        runtime_error(vm, "Invalid null time parameter");
    }
    
    int total_seconds = time->hour * 3600 + time->minute * 60 + time->second + seconds;
    
    // Handle negative values
    while (total_seconds < 0) {
        total_seconds += 24 * 3600;
    }
    
    int new_hour = (total_seconds / 3600) % 24;
    int new_minute = (total_seconds % 3600) / 60;
    int new_second = total_seconds % 60;
    
    return local_time_create(vm, new_hour, new_minute, new_second, time->millis);
}

// ============================================================================
// Local Time comparison functions
// ============================================================================

bool local_time_is_before(const local_time_t* a, const local_time_t* b) {
    if (a == NULL || b == NULL) {
        return false; // Error case
    }
    return local_time_compare(a, b) < 0;
}

bool local_time_is_after(const local_time_t* a, const local_time_t* b) {
    if (a == NULL || b == NULL) {
        return false; // Error case
    }
    return local_time_compare(a, b) > 0;
}

// ============================================================================
// Built-in functions for VM integration
// ============================================================================

value_t builtin_local_date_now(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "LocalDate.now() takes no arguments");
    }

    local_date_t* date = local_date_now(vm);

    value_t val = {0};
    val.type = VAL_LOCAL_DATE;
    val.as.local_date = date;
    val.class = global_local_date_class;
    val.debug = NULL;

    return val;
}

value_t builtin_local_date_of(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 3) {
        runtime_error(vm, "LocalDate.of() requires 3 arguments: year, month, day");
    }

    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error(vm, "LocalDate.of() arguments must be numbers");
    }

    int year = value_to_int(args[0]);
    int month = value_to_int(args[1]);
    int day = value_to_int(args[2]);

    return make_local_date_value(vm, year, month, day);
}

// ============================================================================
// LocalDateTime comparison functions
// ============================================================================

int local_datetime_compare(const local_datetime_t* a, const local_datetime_t* b) {
    if (a == NULL || b == NULL) {
        return (a == NULL) - (b == NULL);
    }
    
    // First compare dates
    int date_cmp = local_date_compare(a->date, b->date);
    if (date_cmp != 0) {
        return date_cmp;
    }
    
    // Dates are equal, compare times
    return local_time_compare(a->time, b->time);
}

bool local_datetime_equals(const local_datetime_t* a, const local_datetime_t* b) {
    if (a == NULL || b == NULL) {
        return a == b;
    }
    return local_date_equals(a->date, b->date) && local_time_equals(a->time, b->time);
}

bool local_datetime_is_before(const local_datetime_t* a, const local_datetime_t* b) {
    return local_datetime_compare(a, b) < 0;
}

bool local_datetime_is_after(const local_datetime_t* a, const local_datetime_t* b) {
    return local_datetime_compare(a, b) > 0;
}

// ============================================================================
// Initialization
// ============================================================================

void init_datetime_classes(vm_t* vm) {
    // Initialize timezone system first
    init_timezone_system();
    
    // Initialize Zone class
    init_zone_class(vm);
    
    // Initialize Date class
    init_date_class(vm);
    
    // Initialize Instant class
    init_instant_class(vm);
    
    // TODO: Initialize other date/time classes when implemented
    // init_local_date_class(vm);
    // init_local_time_class(vm);
    // init_local_datetime_class(vm);
}
