#include "local_date.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "datetime.h"
#include "value.h"
#include "runtime_error.h"
#include "library_assert.h"
#include <string.h>

// External reference to global LocalDate class storage (declared in datetime.c)
extern value_t* global_local_date_class;

// LocalDate factory function
value_t local_date_factory(value_t* args, int arg_count) {
    if (arg_count != 3) {
        runtime_error("LocalDate() requires 3 arguments: year, month, day");
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error("LocalDate() arguments must be numbers");
    }
    
    int year = (int)value_to_double(args[0]);
    int month = (int)value_to_double(args[1]);
    int day = (int)value_to_double(args[2]);
    
    // Validate date components
    if (!is_valid_date(year, month, day)) {
        runtime_error("Invalid date: %d-%02d-%02d", year, month, day);
    }
    
    local_date_t* date = local_date_create(NULL, year, month, day);
    if (!date) {
        if (g_current_vm) {
            slate_runtime_error(g_current_vm, ERR_OOM, __FILE__, __LINE__, -1, 
                               "Memory allocation failed");
        } else {
            fprintf(stderr, "Memory allocation failed\n");
            abort();
        }
    }
    return make_local_date(date);
}

// LocalDate.year() - Get the year
value_t builtin_local_date_year(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.year() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.year() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_year(date));
}

// LocalDate.month() - Get the month
value_t builtin_local_date_month(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.month() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.month() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_month(date));
}

// LocalDate.day() - Get the day
value_t builtin_local_date_day(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.day() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.day() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day(date));
}

// LocalDate.dayOfWeek() - Get day of week (1=Monday, 7=Sunday)
value_t builtin_local_date_day_of_week(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfWeek() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfWeek() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_week(date));
}

// LocalDate.dayOfYear() - Get day of year (1-366)
value_t builtin_local_date_day_of_year(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfYear() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.dayOfYear() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_year(date));
}

// LocalDate.plusDays(days) - Add days to the date
value_t builtin_local_date_plus_days(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusDays() days argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_days(NULL, date, days);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add days to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusMonths(months) - Add months to the date
value_t builtin_local_date_plus_months(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusMonths() months argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_months(NULL, date, months);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add months to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusYears(years) - Add years to the date
value_t builtin_local_date_plus_years(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.plusYears() years argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_years(NULL, date, years);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to add years to date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusDays(days) - Subtract days from the date
value_t builtin_local_date_minus_days(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusDays() days argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_days(NULL, date, -days);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract days from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusMonths(months) - Subtract months from the date
value_t builtin_local_date_minus_months(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusMonths() months argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_months(NULL, date, -months);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract months from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusYears(years) - Subtract years from the date
value_t builtin_local_date_minus_years(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalDate.minusYears() years argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_years(NULL, date, -years);
    if (!new_date) {
        vm_runtime_error_with_debug(vm, "Failed to subtract years from date");
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.equals(other) - Check if dates are equal
value_t builtin_local_date_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.equals() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.equals() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        return make_boolean(0);  // Different types are not equal
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_equals(date1, date2));
}

// LocalDate.isBefore(other) - Check if this date is before another
value_t builtin_local_date_is_before(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isBefore() other argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_before(date1, date2));
}

// LocalDate.isAfter(other) - Check if this date is after another
value_t builtin_local_date_is_after(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.isAfter() other argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_after(date1, date2));
}

// LocalDate.toString() - Convert to string representation
value_t builtin_local_date_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalDate.toString() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        vm_runtime_error_with_debug(vm, "LocalDate.toString() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    char* str = local_date_to_string(NULL, date);
    
    if (!str) {
        vm_runtime_error_with_debug(vm, "Failed to convert date to string");
        return make_null();
    }
    
    value_t result = make_string(str);
    free(str);  // make_string copies the string
    return result;
}


// Initialize LocalDate class with prototype and methods
void local_date_class_init(slate_vm* vm) {
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

    // Create the LocalDate class
    value_t local_date_class = make_class("LocalDate", local_date_proto);
    
    // Set the factory function to allow LocalDate(year, month, day)
    local_date_class.as.class->factory = local_date_factory;
    
    // Add static methods to the LocalDate class
    value_t now_method = make_native(builtin_local_date_now);
    do_set(local_date_class.as.class->properties, "now", &now_method, sizeof(value_t));
    
    value_t of_method = make_native(builtin_local_date_of);
    do_set(local_date_class.as.class->properties, "of", &of_method, sizeof(value_t));
    
    // Store in globals
    do_set(vm->globals, "LocalDate", &local_date_class, sizeof(value_t));

    // Store a global reference for use in make_local_date
    static value_t local_date_class_storage;
    local_date_class_storage = vm_retain(local_date_class);
    global_local_date_class = &local_date_class_storage;
}