#include "local_datetime.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>

// Helper macro to validate LocalDateTime receiver
#define VALIDATE_RECEIVER(vm, args, arg_count, method_name) \
    do { \
        if (arg_count < 1) { \
            runtime_error(vm, method_name "() requires a receiver"); \
            return make_null(); \
        } \
        if (args[0].type != VAL_LOCAL_DATETIME) { \
            runtime_error(vm, method_name "() can only be called on LocalDateTime objects"); \
            return make_null(); \
        } \
    } while(0)

// LocalDateTime method: date()
value_t builtin_local_datetime_date(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.date");
    
    if (arg_count != 1) {
        runtime_error(vm, "date() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_local_date(local_date_retain(dt->date));
}

// LocalDateTime method: time()
value_t builtin_local_datetime_time(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.time");
    
    if (arg_count != 1) {
        runtime_error(vm, "time() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_local_time(local_time_retain(dt->time));
}

// LocalDateTime method: year()
value_t builtin_local_datetime_year(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.year");
    
    if (arg_count != 1) {
        runtime_error(vm, "year() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->date->year);
}

// LocalDateTime method: month()
value_t builtin_local_datetime_month(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.month");
    
    if (arg_count != 1) {
        runtime_error(vm, "month() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->date->month);
}

// LocalDateTime method: day()
value_t builtin_local_datetime_day(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.day");
    
    if (arg_count != 1) {
        runtime_error(vm, "day() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->date->day);
}

// LocalDateTime method: hour()
value_t builtin_local_datetime_hour(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.hour");
    
    if (arg_count != 1) {
        runtime_error(vm, "hour() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->time->hour);
}

// LocalDateTime method: minute()
value_t builtin_local_datetime_minute(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minute");
    
    if (arg_count != 1) {
        runtime_error(vm, "minute() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->time->minute);
}

// LocalDateTime method: second()
value_t builtin_local_datetime_second(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.second");
    
    if (arg_count != 1) {
        runtime_error(vm, "second() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->time->second);
}

// LocalDateTime method: millisecond()
value_t builtin_local_datetime_millisecond(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.millisecond");
    
    if (arg_count != 1) {
        runtime_error(vm, "millisecond() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    return make_int32(dt->time->millis);
}

// LocalDateTime method: plusDays(days)
value_t builtin_local_datetime_plus_days(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusDays");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusDays() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusDays() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int days = value_to_int(args[1]);
    
    // Add days to the date component
    local_date_t* new_date = local_date_plus_days(vm, dt->date, days);
    
    // Create new LocalDateTime with updated date
    local_datetime_t* new_dt = local_datetime_create(vm, new_date, dt->time);
    local_date_release(new_date);
    
    return make_local_datetime(new_dt);
}

// LocalDateTime method: plusMonths(months)
value_t builtin_local_datetime_plus_months(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusMonths");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusMonths() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusMonths() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int months = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_months(vm, dt->date, months);
    local_datetime_t* new_dt = local_datetime_create(vm, new_date, dt->time);
    local_date_release(new_date);
    
    return make_local_datetime(new_dt);
}

// LocalDateTime method: plusYears(years)
value_t builtin_local_datetime_plus_years(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusYears");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusYears() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusYears() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int years = value_to_int(args[1]);
    
    local_date_t* new_date = local_date_plus_years(vm, dt->date, years);
    local_datetime_t* new_dt = local_datetime_create(vm, new_date, dt->time);
    local_date_release(new_date);
    
    return make_local_datetime(new_dt);
}

// LocalDateTime method: plusHours(hours)
value_t builtin_local_datetime_plus_hours(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusHours");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusHours() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusHours() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int hours = value_to_int(args[1]);
    
    // Calculate total hours
    int total_hours = dt->time->hour + hours;
    int day_overflow = 0;
    
    // Handle overflow/underflow
    while (total_hours >= 24) {
        total_hours -= 24;
        day_overflow++;
    }
    while (total_hours < 0) {
        total_hours += 24;
        day_overflow--;
    }
    
    // Create new time with adjusted hours
    local_time_t* new_time = local_time_create(vm, total_hours, dt->time->minute, dt->time->second, dt->time->millis);
    
    // Adjust date if needed
    local_date_t* new_date = dt->date;
    if (day_overflow != 0) {
        new_date = local_date_plus_days(vm, dt->date, day_overflow);
    } else {
        local_date_retain(new_date);
    }
    
    local_datetime_t* new_dt = local_datetime_create(vm, new_date, new_time);
    local_date_release(new_date);
    local_time_release(new_time);
    
    return make_local_datetime(new_dt);
}

// LocalDateTime method: plusMinutes(minutes)
value_t builtin_local_datetime_plus_minutes(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusMinutes");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusMinutes() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusMinutes() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int minutes = value_to_int(args[1]);
    
    // Calculate total minutes
    int total_minutes = dt->time->minute + minutes;
    int hour_overflow = 0;
    
    // Handle overflow/underflow
    while (total_minutes >= 60) {
        total_minutes -= 60;
        hour_overflow++;
    }
    while (total_minutes < 0) {
        total_minutes += 60;
        hour_overflow--;
    }
    
    // Create new time with adjusted minutes
    local_time_t* new_time = local_time_create(vm, dt->time->hour, total_minutes, dt->time->second, dt->time->millis);
    local_datetime_t* temp_dt = local_datetime_create(vm, dt->date, new_time);
    local_time_release(new_time);
    
    // Adjust hours if needed
    if (hour_overflow != 0) {
        value_t temp_val = make_local_datetime(temp_dt);
        value_t hours_val = make_int32(hour_overflow);
        value_t args_array[2] = {temp_val, hours_val};
        value_t result = builtin_local_datetime_plus_hours(vm, 2, args_array);
        local_datetime_release(temp_dt);
        return result;
    }
    
    return make_local_datetime(temp_dt);
}

// LocalDateTime method: plusSeconds(seconds)
value_t builtin_local_datetime_plus_seconds(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.plusSeconds");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusSeconds() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusSeconds() argument must be an integer");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    int seconds = value_to_int(args[1]);
    
    // Calculate total seconds
    int total_seconds = dt->time->second + seconds;
    int minute_overflow = 0;
    
    // Handle overflow/underflow
    while (total_seconds >= 60) {
        total_seconds -= 60;
        minute_overflow++;
    }
    while (total_seconds < 0) {
        total_seconds += 60;
        minute_overflow--;
    }
    
    // Create new time with adjusted seconds
    local_time_t* new_time = local_time_create(vm, dt->time->hour, dt->time->minute, total_seconds, dt->time->millis);
    local_datetime_t* temp_dt = local_datetime_create(vm, dt->date, new_time);
    local_time_release(new_time);
    
    // Adjust minutes if needed
    if (minute_overflow != 0) {
        value_t temp_val = make_local_datetime(temp_dt);
        value_t minutes_val = make_int32(minute_overflow);
        value_t args_array[2] = {temp_val, minutes_val};
        value_t result = builtin_local_datetime_plus_minutes(vm, 2, args_array);
        local_datetime_release(temp_dt);
        return result;
    }
    
    return make_local_datetime(temp_dt);
}

// LocalDateTime method: minusDays(days)
value_t builtin_local_datetime_minus_days(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusDays");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusDays() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusDays() argument must be an integer");
        return make_null();
    }
    
    // Reuse plusDays with negative value
    value_t neg_days = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_days};
    return builtin_local_datetime_plus_days(vm, 2, args_array);
}

// LocalDateTime method: minusMonths(months)
value_t builtin_local_datetime_minus_months(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusMonths");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusMonths() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusMonths() argument must be an integer");
        return make_null();
    }
    
    value_t neg_months = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_months};
    return builtin_local_datetime_plus_months(vm, 2, args_array);
}

// LocalDateTime method: minusYears(years)
value_t builtin_local_datetime_minus_years(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusYears");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusYears() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusYears() argument must be an integer");
        return make_null();
    }
    
    value_t neg_years = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_years};
    return builtin_local_datetime_plus_years(vm, 2, args_array);
}

// LocalDateTime method: minusHours(hours)
value_t builtin_local_datetime_minus_hours(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusHours");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusHours() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusHours() argument must be an integer");
        return make_null();
    }
    
    value_t neg_hours = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_hours};
    return builtin_local_datetime_plus_hours(vm, 2, args_array);
}

// LocalDateTime method: minusMinutes(minutes)
value_t builtin_local_datetime_minus_minutes(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusMinutes");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusMinutes() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusMinutes() argument must be an integer");
        return make_null();
    }
    
    value_t neg_minutes = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_minutes};
    return builtin_local_datetime_plus_minutes(vm, 2, args_array);
}

// LocalDateTime method: minusSeconds(seconds)
value_t builtin_local_datetime_minus_seconds(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.minusSeconds");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusSeconds() takes exactly 1 argument");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusSeconds() argument must be an integer");
        return make_null();
    }
    
    value_t neg_seconds = make_int32(-value_to_int(args[1]));
    value_t args_array[2] = {args[0], neg_seconds};
    return builtin_local_datetime_plus_seconds(vm, 2, args_array);
}

// LocalDateTime method: equals(other)
value_t builtin_local_datetime_equals(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.equals");
    
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATETIME) {
        return make_boolean(0); // Different types are not equal
    }
    
    local_datetime_t* dt1 = args[0].as.local_datetime;
    local_datetime_t* dt2 = args[1].as.local_datetime;
    
    // Compare both date and time components
    bool dates_equal = local_date_equals(dt1->date, dt2->date);
    bool times_equal = local_time_equals(dt1->time, dt2->time);
    
    return make_boolean(dates_equal && times_equal);
}

// LocalDateTime method: isBefore(other)
value_t builtin_local_datetime_is_before(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.isBefore");
    
    if (arg_count != 2) {
        runtime_error(vm, "isBefore() takes exactly 1 argument");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATETIME) {
        runtime_error(vm, "isBefore() argument must be a LocalDateTime");
        return make_null();
    }
    
    local_datetime_t* dt1 = args[0].as.local_datetime;
    local_datetime_t* dt2 = args[1].as.local_datetime;
    
    // First compare dates
    int date_cmp = local_date_compare(dt1->date, dt2->date);
    if (date_cmp < 0) return make_boolean(1);
    if (date_cmp > 0) return make_boolean(0);
    
    // Dates are equal, compare times
    return make_boolean(local_time_is_before(dt1->time, dt2->time));
}

// LocalDateTime method: isAfter(other)
value_t builtin_local_datetime_is_after(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.isAfter");
    
    if (arg_count != 2) {
        runtime_error(vm, "isAfter() takes exactly 1 argument");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATETIME) {
        runtime_error(vm, "isAfter() argument must be a LocalDateTime");
        return make_null();
    }
    
    local_datetime_t* dt1 = args[0].as.local_datetime;
    local_datetime_t* dt2 = args[1].as.local_datetime;
    
    // First compare dates
    int date_cmp = local_date_compare(dt1->date, dt2->date);
    if (date_cmp > 0) return make_boolean(1);
    if (date_cmp < 0) return make_boolean(0);
    
    // Dates are equal, compare times
    return make_boolean(local_time_is_after(dt1->time, dt2->time));
}

// LocalDateTime method: toString()
value_t builtin_local_datetime_to_string(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_RECEIVER(vm, args, arg_count, "LocalDateTime.toString");
    
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = args[0].as.local_datetime;
    
    // Format as ISO 8601: YYYY-MM-DDTHH:mm:ss or YYYY-MM-DDTHH:mm:ss.SSS
    char buffer[30];
    if (dt->time->millis > 0) {
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%03d",
                dt->date->year, dt->date->month, dt->date->day,
                dt->time->hour, dt->time->minute, dt->time->second, dt->time->millis);
    } else {
        snprintf(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d",
                dt->date->year, dt->date->month, dt->date->day,
                dt->time->hour, dt->time->minute, dt->time->second);
    }
    
    return make_string(buffer);
}

// LocalDateTime static method: now()
value_t builtin_local_datetime_now(vm_t* vm, int arg_count, value_t* args) {
    (void)args; // Unused
    
    if (arg_count != 0) {
        runtime_error(vm, "LocalDateTime.now() takes no arguments");
        return make_null();
    }
    
    local_datetime_t* dt = local_datetime_now(vm);
    return make_local_datetime(dt);
}