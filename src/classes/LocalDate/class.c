#include "local_date.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "dynamic_object.h"

// External reference to global LocalDate class storage (declared in datetime.c)
extern value_t* global_local_date_class;

// Initialize LocalDate class with prototype and methods
void local_date_class_init(vm_t* vm) {
    // Create the LocalDate class with its prototype
    do_object local_date_proto = do_create(NULL);

    // Add methods to LocalDate prototype
    value_t year_method = make_native(builtin_local_date_year);
    do_set(local_date_proto, "year", &year_method, sizeof(value_t));

    value_t month_method = make_native(builtin_local_date_month);
    do_set(local_date_proto, "month", &month_method, sizeof(value_t));

    value_t day_method = make_native(builtin_local_date_day);
    do_set(local_date_proto, "day", &day_method, sizeof(value_t));

    value_t day_of_week_method = make_native(builtin_local_date_day_of_week);
    do_set(local_date_proto, "dayOfWeek", &day_of_week_method, sizeof(value_t));

    value_t day_of_year_method = make_native(builtin_local_date_day_of_year);
    do_set(local_date_proto, "dayOfYear", &day_of_year_method, sizeof(value_t));

    value_t plus_days_method = make_native(builtin_local_date_plus_days);
    do_set(local_date_proto, "plusDays", &plus_days_method, sizeof(value_t));

    value_t plus_months_method = make_native(builtin_local_date_plus_months);
    do_set(local_date_proto, "plusMonths", &plus_months_method, sizeof(value_t));

    value_t plus_years_method = make_native(builtin_local_date_plus_years);
    do_set(local_date_proto, "plusYears", &plus_years_method, sizeof(value_t));

    value_t minus_days_method = make_native(builtin_local_date_minus_days);
    do_set(local_date_proto, "minusDays", &minus_days_method, sizeof(value_t));

    value_t minus_months_method = make_native(builtin_local_date_minus_months);
    do_set(local_date_proto, "minusMonths", &minus_months_method, sizeof(value_t));

    value_t minus_years_method = make_native(builtin_local_date_minus_years);
    do_set(local_date_proto, "minusYears", &minus_years_method, sizeof(value_t));

    value_t equals_method = make_native(builtin_local_date_equals);
    do_set(local_date_proto, "equals", &equals_method, sizeof(value_t));

    value_t is_before_method = make_native(builtin_local_date_is_before);
    do_set(local_date_proto, "isBefore", &is_before_method, sizeof(value_t));

    value_t is_after_method = make_native(builtin_local_date_is_after);
    do_set(local_date_proto, "isAfter", &is_after_method, sizeof(value_t));

    value_t to_string_method = make_native(builtin_local_date_to_string);
    do_set(local_date_proto, "toString", &to_string_method, sizeof(value_t));

    // Create static methods object
    do_object local_date_static = do_create(NULL);
    value_t now_method = make_native(builtin_local_date_now);
    do_set(local_date_static, "now", &now_method, sizeof(value_t));
    
    value_t of_method = make_native(builtin_local_date_of);
    do_set(local_date_static, "of", &of_method, sizeof(value_t));
    
    // Create the LocalDate class
    value_t local_date_class = make_class("LocalDate", local_date_proto, local_date_static);
    
    // Set the factory function to allow LocalDate(year, month, day)
    local_date_class.as.class->factory = local_date_factory;
    
    // Store in globals
    do_set(vm->globals, "LocalDate", &local_date_class, sizeof(value_t));

    // Store a global reference for use in make_local_date
    static value_t local_date_class_storage;
    local_date_class_storage = vm_retain(local_date_class);
    global_local_date_class = &local_date_class_storage;
}