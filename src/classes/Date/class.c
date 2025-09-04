#include "date_class.h"
#include "value.h"
#include "vm.h"
#include "dynamic_object.h"

// Global reference to Date class (declared in datetime.c)
extern value_t* global_date_class;

// Initialize Date class with prototype and methods
void init_date_class(vm_t* vm) {
    // Create the Date class with its prototype
    do_object date_proto = do_create(NULL);
    
    // Add instance methods to Date prototype
    value_t local_datetime_method = make_native(date_get_local_datetime_method);
    do_set(date_proto, "localDateTime", &local_datetime_method, sizeof(value_t));
    
    value_t zone_method = make_native(date_get_zone_method);
    do_set(date_proto, "zone", &zone_method, sizeof(value_t));
    
    value_t to_instant_method = make_native(date_to_instant_method);
    do_set(date_proto, "toInstant", &to_instant_method, sizeof(value_t));
    
    value_t with_zone_method = make_native(date_with_zone_method);
    do_set(date_proto, "withZone", &with_zone_method, sizeof(value_t));
    
    value_t at_zone_method = make_native(date_with_zone_method);
    do_set(date_proto, "atZone", &at_zone_method, sizeof(value_t));
    
    value_t with_local_datetime_method = make_native(date_with_local_datetime_method);
    do_set(date_proto, "withLocalDateTime", &with_local_datetime_method, sizeof(value_t));
    
    // Date arithmetic methods
    value_t plus_hours_method = make_native(date_plus_hours_method);
    do_set(date_proto, "plusHours", &plus_hours_method, sizeof(value_t));
    
    value_t plus_minutes_method = make_native(date_plus_minutes_method);
    do_set(date_proto, "plusMinutes", &plus_minutes_method, sizeof(value_t));
    
    value_t plus_seconds_method = make_native(date_plus_seconds_method);
    do_set(date_proto, "plusSeconds", &plus_seconds_method, sizeof(value_t));
    
    value_t plus_days_method = make_native(date_plus_days_method);
    do_set(date_proto, "plusDays", &plus_days_method, sizeof(value_t));
    
    value_t plus_months_method = make_native(date_plus_months_method);
    do_set(date_proto, "plusMonths", &plus_months_method, sizeof(value_t));
    
    value_t plus_years_method = make_native(date_plus_years_method);
    do_set(date_proto, "plusYears", &plus_years_method, sizeof(value_t));
    
    // Comparison methods
    value_t is_before_method = make_native(date_is_before_method);
    do_set(date_proto, "isBefore", &is_before_method, sizeof(value_t));
    
    value_t is_after_method = make_native(date_is_after_method);
    do_set(date_proto, "isAfter", &is_after_method, sizeof(value_t));
    
    value_t equals_method = make_native(date_equals_method);
    do_set(date_proto, "equals", &equals_method, sizeof(value_t));
    
    value_t to_string_method = make_native(date_to_string_method);
    do_set(date_proto, "toString", &to_string_method, sizeof(value_t));
    
    // Create static methods object
    do_object date_static = do_create(NULL);
    value_t now_method = make_native(date_now_factory);
    do_set(date_static, "now", &now_method, sizeof(value_t));
    
    value_t now_in_zone_method = make_native(date_now_in_zone_factory);
    do_set(date_static, "nowInZone", &now_in_zone_method, sizeof(value_t));
    
    value_t of_method = make_native(date_of_factory);
    do_set(date_static, "of", &of_method, sizeof(value_t));
    
    value_t from_instant_method = make_native(date_from_instant_factory);
    do_set(date_static, "fromInstant", &from_instant_method, sizeof(value_t));
    
    value_t parse_method = make_native(date_parse);
    do_set(date_static, "parse", &parse_method, sizeof(value_t));
    
    // Create the Date class
    value_t date_class = make_class("Date", date_proto, date_static);
    
    // Set the factory function to allow Date constructor calls
    date_class.as.class->factory = date_factory;
    
    // Store in globals
    do_set(vm->globals, "Date", &date_class, sizeof(value_t));
    
    // Store a global reference for use in make_date_direct
    static value_t date_class_storage;
    date_class_storage = vm_retain(date_class);
    global_date_class = &date_class_storage;
}

// Utility functions for creating Date values
value_t make_date_direct(date_t* date) {
    return make_date(date);
}

value_t make_date_direct_with_debug(date_t* date, debug_location* debug) {
    return make_date_with_debug(date, debug);
}