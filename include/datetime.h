#ifndef SLATE_DATETIME_H
#define SLATE_DATETIME_H

#include "vm.h"

// Global date/time class references
extern value_t* global_local_date_class;
extern value_t* global_local_time_class;
extern value_t* global_local_datetime_class;
extern value_t* global_date_class;
extern value_t* global_instant_class;
extern value_t* global_duration_class;
extern value_t* global_period_class;

// Date/time validation functions
bool is_valid_date(int year, int month, int day);
bool is_valid_time(int hour, int minute, int second, int millis);
bool is_leap_year(int year);
int days_in_month(int year, int month);
int days_in_year(int year);
uint32_t date_to_epoch_day(int year, int month, int day);
void epoch_day_to_date(uint32_t epoch_day, int* year, int* month, int* day);

// Local Date factory functions
local_date_t* local_date_create(vm_t* vm, int year, int month, int day);
local_date_t* local_date_now(vm_t* vm);
local_date_t* local_date_of_epoch_day(vm_t* vm, uint32_t epoch_day);

// Local Time factory functions
local_time_t* local_time_create(vm_t* vm, int hour, int minute, int second, int millis);
local_time_t* local_time_now(vm_t* vm);

// Local DateTime factory functions
local_datetime_t* local_datetime_create(vm_t* vm, local_date_t* date, local_time_t* time);
local_datetime_t* local_datetime_now(vm_t* vm);

// Instant factory functions - now handled by Instant class

// Duration factory functions
duration_t* duration_create(int64_t seconds, int32_t nanos);
duration_t* duration_of_seconds(int64_t seconds);
duration_t* duration_of_minutes(int64_t minutes);
duration_t* duration_of_hours(int64_t hours);
duration_t* duration_of_days(int64_t days);

// Period factory functions
period_t* period_create(int years, int months, int days);
period_t* period_of_years(int years);
period_t* period_of_months(int months);
period_t* period_of_days(int days);

// Value factory functions (create value_t wrappers)
value_t make_local_date_value(vm_t* vm, int year, int month, int day);
value_t make_local_time_value(vm_t* vm, int hour, int minute, int second, int millis);
value_t make_local_datetime_value(vm_t* vm, local_date_t* date, local_time_t* time);
// make_instant_value - now handled by make_instant_direct
value_t make_duration_value(int64_t seconds, int32_t nanos);
value_t make_period_value(int years, int months, int days);

// Memory management functions
local_date_t* local_date_retain(local_date_t* date);
void local_date_release(local_date_t* date);
local_time_t* local_time_retain(local_time_t* time);
void local_time_release(local_time_t* time);
local_datetime_t* local_datetime_retain(local_datetime_t* dt);
void local_datetime_release(local_datetime_t* dt);
// zoned_datetime functions removed - use Date class instead
instant_t* instant_retain(instant_t* instant);
void instant_release(instant_t* instant);
duration_t* duration_retain(duration_t* duration);
void duration_release(duration_t* duration);
period_t* period_retain(period_t* period);
void period_release(period_t* period);

// Comparison functions
int local_date_compare(const local_date_t* a, const local_date_t* b);
bool local_date_equals(const local_date_t* a, const local_date_t* b);
bool local_date_is_before(const local_date_t* a, const local_date_t* b);
bool local_date_is_after(const local_date_t* a, const local_date_t* b);

int local_time_compare(const local_time_t* a, const local_time_t* b);
bool local_time_equals(const local_time_t* a, const local_time_t* b);
bool local_time_is_before(const local_time_t* a, const local_time_t* b);
bool local_time_is_after(const local_time_t* a, const local_time_t* b);

int local_datetime_compare(const local_datetime_t* a, const local_datetime_t* b);
bool local_datetime_equals(const local_datetime_t* a, const local_datetime_t* b);
bool local_datetime_is_before(const local_datetime_t* a, const local_datetime_t* b);
bool local_datetime_is_after(const local_datetime_t* a, const local_datetime_t* b);

// Date arithmetic functions
local_date_t* local_date_plus_days(vm_t* vm, const local_date_t* date, int days);
local_date_t* local_date_plus_months(vm_t* vm, const local_date_t* date, int months);
local_date_t* local_date_plus_years(vm_t* vm, const local_date_t* date, int years);

// Time arithmetic functions
local_time_t* local_time_plus_hours(vm_t* vm, const local_time_t* time, int hours);
local_time_t* local_time_plus_minutes(vm_t* vm, const local_time_t* time, int minutes);
local_time_t* local_time_plus_seconds(vm_t* vm, const local_time_t* time, int seconds);

// Date accessor functions
int local_date_get_year(const local_date_t* date);
int local_date_get_month(const local_date_t* date);
int local_date_get_day(const local_date_t* date);
int local_date_get_day_of_week(const local_date_t* date); // 1=Monday, 7=Sunday
int local_date_get_day_of_year(const local_date_t* date);

// Date modifier functions (create new instances)
local_date_t* local_date_with_year(const local_date_t* date, int year);
local_date_t* local_date_with_month(const local_date_t* date, int month);
local_date_t* local_date_with_day(const local_date_t* date, int day);

// Time accessor functions
int local_time_get_hour(const local_time_t* time);
int local_time_get_minute(const local_time_t* time);
int local_time_get_second(const local_time_t* time);
int local_time_get_millisecond(const local_time_t* time);

// Time modifier functions (create new instances)
local_time_t* local_time_with_hour(const local_time_t* time, int hour);
local_time_t* local_time_with_minute(const local_time_t* time, int minute);
local_time_t* local_time_with_second(const local_time_t* time, int second);

// String conversion functions
char* local_date_to_string(vm_t* vm, const local_date_t* date);
char* local_time_to_string(vm_t* vm, const local_time_t* time);
char* local_datetime_to_string(vm_t* vm, const local_datetime_t* dt);

// Parsing functions
local_date_t* local_date_parse_iso(const char* iso_string);
local_time_t* local_time_parse_iso(const char* iso_string);
local_datetime_t* local_datetime_parse_iso(vm_t* vm, const char* iso_string);

// Built-in function prototypes for VM integration
value_t builtin_local_date_now(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_of(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_date_parse(vm_t* vm, int arg_count, value_t* args);

// Initialization function
void init_datetime_classes(vm_t* vm);

#endif // SLATE_DATETIME_H