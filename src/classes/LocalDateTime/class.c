#include "local_datetime.h"
#include "value.h"
#include "vm.h"
#include "dynamic_object.h"

// Initialize LocalDateTime class with prototype and methods
void local_datetime_class_init(vm_t* vm) {
    // Create the LocalDateTime class with its prototype
    do_object local_datetime_proto = do_create(NULL);
    
    // Add instance methods to LocalDateTime prototype
    value_t date_method = make_native(builtin_local_datetime_date);
    do_set(local_datetime_proto, "date", &date_method, sizeof(value_t));
    
    value_t time_method = make_native(builtin_local_datetime_time);
    do_set(local_datetime_proto, "time", &time_method, sizeof(value_t));
    
    value_t year_method = make_native(builtin_local_datetime_year);
    do_set(local_datetime_proto, "year", &year_method, sizeof(value_t));
    
    value_t month_method = make_native(builtin_local_datetime_month);
    do_set(local_datetime_proto, "month", &month_method, sizeof(value_t));
    
    value_t day_method = make_native(builtin_local_datetime_day);
    do_set(local_datetime_proto, "day", &day_method, sizeof(value_t));
    
    value_t hour_method = make_native(builtin_local_datetime_hour);
    do_set(local_datetime_proto, "hour", &hour_method, sizeof(value_t));
    
    value_t minute_method = make_native(builtin_local_datetime_minute);
    do_set(local_datetime_proto, "minute", &minute_method, sizeof(value_t));
    
    value_t second_method = make_native(builtin_local_datetime_second);
    do_set(local_datetime_proto, "second", &second_method, sizeof(value_t));
    
    value_t millisecond_method = make_native(builtin_local_datetime_millisecond);
    do_set(local_datetime_proto, "millisecond", &millisecond_method, sizeof(value_t));
    
    value_t plus_days_method = make_native(builtin_local_datetime_plus_days);
    do_set(local_datetime_proto, "plusDays", &plus_days_method, sizeof(value_t));
    
    value_t plus_months_method = make_native(builtin_local_datetime_plus_months);
    do_set(local_datetime_proto, "plusMonths", &plus_months_method, sizeof(value_t));
    
    value_t plus_years_method = make_native(builtin_local_datetime_plus_years);
    do_set(local_datetime_proto, "plusYears", &plus_years_method, sizeof(value_t));
    
    value_t plus_hours_method = make_native(builtin_local_datetime_plus_hours);
    do_set(local_datetime_proto, "plusHours", &plus_hours_method, sizeof(value_t));
    
    value_t plus_minutes_method = make_native(builtin_local_datetime_plus_minutes);
    do_set(local_datetime_proto, "plusMinutes", &plus_minutes_method, sizeof(value_t));
    
    value_t plus_seconds_method = make_native(builtin_local_datetime_plus_seconds);
    do_set(local_datetime_proto, "plusSeconds", &plus_seconds_method, sizeof(value_t));
    
    value_t minus_days_method = make_native(builtin_local_datetime_minus_days);
    do_set(local_datetime_proto, "minusDays", &minus_days_method, sizeof(value_t));
    
    value_t minus_months_method = make_native(builtin_local_datetime_minus_months);
    do_set(local_datetime_proto, "minusMonths", &minus_months_method, sizeof(value_t));
    
    value_t minus_years_method = make_native(builtin_local_datetime_minus_years);
    do_set(local_datetime_proto, "minusYears", &minus_years_method, sizeof(value_t));
    
    value_t minus_hours_method = make_native(builtin_local_datetime_minus_hours);
    do_set(local_datetime_proto, "minusHours", &minus_hours_method, sizeof(value_t));
    
    value_t minus_minutes_method = make_native(builtin_local_datetime_minus_minutes);
    do_set(local_datetime_proto, "minusMinutes", &minus_minutes_method, sizeof(value_t));
    
    value_t minus_seconds_method = make_native(builtin_local_datetime_minus_seconds);
    do_set(local_datetime_proto, "minusSeconds", &minus_seconds_method, sizeof(value_t));
    
    value_t equals_method = make_native(builtin_local_datetime_equals);
    do_set(local_datetime_proto, "equals", &equals_method, sizeof(value_t));
    
    value_t is_before_method = make_native(builtin_local_datetime_is_before);
    do_set(local_datetime_proto, "isBefore", &is_before_method, sizeof(value_t));
    
    value_t is_after_method = make_native(builtin_local_datetime_is_after);
    do_set(local_datetime_proto, "isAfter", &is_after_method, sizeof(value_t));
    
    value_t to_string_method = make_native(builtin_local_datetime_to_string);
    do_set(local_datetime_proto, "toString", &to_string_method, sizeof(value_t));
    
    // Create static methods object
    do_object local_datetime_static = do_create(NULL);
    value_t now_method = make_native(builtin_local_datetime_now);
    do_set(local_datetime_static, "now", &now_method, sizeof(value_t));
    
    // Create the LocalDateTime class
    value_t local_datetime_class = make_class("LocalDateTime", local_datetime_proto, local_datetime_static);
    
    // Set the factory function to allow LocalDateTime constructor calls
    local_datetime_class.as.class->factory = local_datetime_factory;
    
    // Store in globals
    do_set(vm->globals, "LocalDateTime", &local_datetime_class, sizeof(value_t));
    
    // Store a global reference for use in make_local_datetime
    static value_t local_datetime_class_storage;
    local_datetime_class_storage = vm_retain(local_datetime_class);
    global_local_datetime_class = &local_datetime_class_storage;
}