#include "local_date.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "runtime_error.h"
#include <string.h>

// LocalDate.year() - Get the year
value_t builtin_local_date_year(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.year() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.year() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_year(date));
}

// LocalDate.month() - Get the month
value_t builtin_local_date_month(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.month() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.month() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_month(date));
}

// LocalDate.day() - Get the day
value_t builtin_local_date_day(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.day() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.day() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day(date));
}

// LocalDate.dayOfWeek() - Get day of week (1=Monday, 7=Sunday)
value_t builtin_local_date_day_of_week(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.dayOfWeek() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.dayOfWeek() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_week(date));
}

// LocalDate.dayOfYear() - Get day of year (1-366)
value_t builtin_local_date_day_of_year(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.dayOfYear() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.dayOfYear() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    return make_int32(local_date_get_day_of_year(date));
}

// LocalDate.plusDays(days) - Add days to the date
value_t builtin_local_date_plus_days(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.plusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.plusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.plusDays() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_days(vm, date, days);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusMonths(months) - Add months to the date
value_t builtin_local_date_plus_months(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.plusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.plusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.plusMonths() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_months(vm, date, months);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.plusYears(years) - Add years to the date
value_t builtin_local_date_plus_years(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.plusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.plusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.plusYears() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_years(vm, date, years);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusDays(days) - Subtract days from the date
value_t builtin_local_date_minus_days(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.minusDays() takes 2 arguments (self, days)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.minusDays() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.minusDays() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int days = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_days(vm, date, -days);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusMonths(months) - Subtract months from the date
value_t builtin_local_date_minus_months(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.minusMonths() takes 2 arguments (self, months)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.minusMonths() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.minusMonths() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int months = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_months(vm, date, -months);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.minusYears(years) - Subtract years from the date
value_t builtin_local_date_minus_years(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.minusYears() takes 2 arguments (self, years)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.minusYears() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        runtime_error(vm, "LocalDate.minusYears() second argument must be a number");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    int years = (int)value_to_float64(args[1]);
    
    local_date_t* new_date = local_date_plus_years(vm, date, -years);
    if (!new_date) {
        return make_null();
    }
    
    return make_local_date(new_date);
}

// LocalDate.equals(other) - Check if dates are equal
value_t builtin_local_date_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.equals() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.equals() can only be called on LocalDate objects");
        return make_null();
    }
    
    // Return false if other is not a LocalDate
    if (args[1].type != VAL_LOCAL_DATE) {
        return make_boolean(false);
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_equals(date1, date2));
}

// LocalDate.isBefore(other) - Check if this date is before another
value_t builtin_local_date_is_before(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.isBefore() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.isBefore() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.isBefore() second argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_before(date1, date2));
}

// LocalDate.isAfter(other) - Check if this date is after another
value_t builtin_local_date_is_after(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "LocalDate.isAfter() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.isAfter() can only be called on LocalDate objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.isAfter() second argument must be a LocalDate");
        return make_null();
    }
    
    local_date_t* date1 = args[0].as.local_date;
    local_date_t* date2 = args[1].as.local_date;
    
    return make_boolean(local_date_is_after(date1, date2));
}

// LocalDate.toString() - Convert to string representation
value_t builtin_local_date_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "LocalDate.toString() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_DATE) {
        runtime_error(vm, "LocalDate.toString() can only be called on LocalDate objects");
        return make_null();
    }
    
    local_date_t* date = args[0].as.local_date;
    char* date_str = local_date_to_string(vm, date);
    
    if (!date_str) {
        return make_null();
    }
    
    // Create dynamic string from the C string
    value_t result = make_string(date_str);
    free(date_str);
    
    return result;
}