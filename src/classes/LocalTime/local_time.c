#include "local_time.h"
#include "builtins.h"
#include "dynamic_object.h"
#include "datetime.h"
#include "value.h"
#include <assert.h>
#include <string.h>

// External reference to global LocalTime class storage (declared in datetime.c)
extern value_t* global_local_time_class;

// LocalTime factory function
value_t local_time_factory(value_t* args, int arg_count) {
    if (arg_count != 3 && arg_count != 4) {
        runtime_error("LocalTime() requires 3 or 4 arguments: hour, minute, second, [millisecond]");
    }
    
    // Validate arguments are numbers
    if (!is_number(args[0]) || !is_number(args[1]) || !is_number(args[2])) {
        runtime_error("LocalTime() first 3 arguments must be numbers");
    }
    
    if (arg_count == 4 && !is_number(args[3])) {
        runtime_error("LocalTime() millisecond argument must be a number");
    }
    
    int hour = (int)value_to_double(args[0]);
    int minute = (int)value_to_double(args[1]);
    int second = (int)value_to_double(args[2]);
    int millis = (arg_count == 4) ? (int)value_to_double(args[3]) : 0;
    
    // Validate time components
    if (!is_valid_time(hour, minute, second, millis)) {
        runtime_error("Invalid time: %02d:%02d:%02d.%03d", hour, minute, second, millis);
    }
    
    local_time_t* time = local_time_create(NULL, hour, minute, second, millis);
    assert(time != NULL); // Per user: allocation failures are assertion failures
    return make_local_time(time);
}

// LocalTime.hour() - Get the hour
value_t builtin_local_time_hour(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.hour() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.hour() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_hour(time));
}

// LocalTime.minute() - Get the minute
value_t builtin_local_time_minute(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.minute() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minute() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_minute(time));
}

// LocalTime.second() - Get the second
value_t builtin_local_time_second(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.second() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.second() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_second(time));
}

// LocalTime.millisecond() - Get the millisecond
value_t builtin_local_time_millisecond(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.millisecond() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.millisecond() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    return make_int32(local_time_get_millisecond(time));
}

// LocalTime.plusHours(hours) - Add hours to the time
value_t builtin_local_time_plus_hours(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() takes 2 arguments (self, hours)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusHours() hours argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int hours = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_hours(NULL, time, hours);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.plusMinutes(minutes) - Add minutes to the time
value_t builtin_local_time_plus_minutes(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() takes 2 arguments (self, minutes)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusMinutes() minutes argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int minutes = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_minutes(NULL, time, minutes);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.plusSeconds(seconds) - Add seconds to the time
value_t builtin_local_time_plus_seconds(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() takes 2 arguments (self, seconds)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.plusSeconds() seconds argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int seconds = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_seconds(NULL, time, seconds);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusHours(hours) - Subtract hours from the time
value_t builtin_local_time_minus_hours(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() takes 2 arguments (self, hours)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusHours() hours argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int hours = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_hours(NULL, time, -hours);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusMinutes(minutes) - Subtract minutes from the time
value_t builtin_local_time_minus_minutes(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() takes 2 arguments (self, minutes)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusMinutes() minutes argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int minutes = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_minutes(NULL, time, -minutes);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.minusSeconds(seconds) - Subtract seconds from the time
value_t builtin_local_time_minus_seconds(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() takes 2 arguments (self, seconds)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (!is_number(args[1])) {
        vm_runtime_error_with_debug(vm, "LocalTime.minusSeconds() seconds argument must be a number");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    int seconds = value_to_int(args[1]);
    
    local_time_t* new_time = local_time_plus_seconds(NULL, time, -seconds);
    assert(new_time != NULL);
    
    return make_local_time(new_time);
}

// LocalTime.equals(other) - Check if times are equal
value_t builtin_local_time_equals(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.equals() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.equals() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        return make_boolean(false);
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_equals(time1, time2));
}

// LocalTime.isBefore(other) - Check if this time is before another
value_t builtin_local_time_is_before(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isBefore() other argument must be a LocalTime");
        return make_null();
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_is_before(time1, time2));
}

// LocalTime.isAfter(other) - Check if this time is after another
value_t builtin_local_time_is_after(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() takes 2 arguments (self, other)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() can only be called on LocalTime objects");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.isAfter() other argument must be a LocalTime");
        return make_null();
    }
    
    local_time_t* time1 = args[0].as.local_time;
    local_time_t* time2 = args[1].as.local_time;
    
    return make_boolean(local_time_is_after(time1, time2));
}

// LocalTime.toString() - Convert to string representation
value_t builtin_local_time_to_string(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        vm_runtime_error_with_debug(vm, "LocalTime.toString() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_LOCAL_TIME) {
        vm_runtime_error_with_debug(vm, "LocalTime.toString() can only be called on LocalTime objects");
        return make_null();
    }
    
    local_time_t* time = args[0].as.local_time;
    char* str = local_time_to_string(NULL, time);
    
    assert(str != NULL);
    
    value_t result = make_string(str);
    free(str);  // make_string copies the string
    
    return result;
}

// Initialize LocalTime class with prototype and methods
void local_time_class_init(slate_vm* vm) {
    // Create the LocalTime class with its prototype
    do_object local_time_proto = do_create(NULL);

    // Add methods to LocalTime prototype
    value_t hour_method = make_native(builtin_local_time_hour);
    do_set(local_time_proto, "hour", &hour_method, sizeof(value_t));

    value_t minute_method = make_native(builtin_local_time_minute);
    do_set(local_time_proto, "minute", &minute_method, sizeof(value_t));

    value_t second_method = make_native(builtin_local_time_second);
    do_set(local_time_proto, "second", &second_method, sizeof(value_t));

    value_t millisecond_method = make_native(builtin_local_time_millisecond);
    do_set(local_time_proto, "millisecond", &millisecond_method, sizeof(value_t));

    value_t plus_hours_method = make_native(builtin_local_time_plus_hours);
    do_set(local_time_proto, "plusHours", &plus_hours_method, sizeof(value_t));

    value_t plus_minutes_method = make_native(builtin_local_time_plus_minutes);
    do_set(local_time_proto, "plusMinutes", &plus_minutes_method, sizeof(value_t));

    value_t plus_seconds_method = make_native(builtin_local_time_plus_seconds);
    do_set(local_time_proto, "plusSeconds", &plus_seconds_method, sizeof(value_t));

    value_t minus_hours_method = make_native(builtin_local_time_minus_hours);
    do_set(local_time_proto, "minusHours", &minus_hours_method, sizeof(value_t));

    value_t minus_minutes_method = make_native(builtin_local_time_minus_minutes);
    do_set(local_time_proto, "minusMinutes", &minus_minutes_method, sizeof(value_t));

    value_t minus_seconds_method = make_native(builtin_local_time_minus_seconds);
    do_set(local_time_proto, "minusSeconds", &minus_seconds_method, sizeof(value_t));

    value_t time_equals_method = make_native(builtin_local_time_equals);
    do_set(local_time_proto, "equals", &time_equals_method, sizeof(value_t));

    value_t time_is_before_method = make_native(builtin_local_time_is_before);
    do_set(local_time_proto, "isBefore", &time_is_before_method, sizeof(value_t));

    value_t time_is_after_method = make_native(builtin_local_time_is_after);
    do_set(local_time_proto, "isAfter", &time_is_after_method, sizeof(value_t));

    value_t time_to_string_method = make_native(builtin_local_time_to_string);
    do_set(local_time_proto, "toString", &time_to_string_method, sizeof(value_t));

    // Create the LocalTime class
    value_t local_time_class = make_class("LocalTime", local_time_proto);
    
    // Set the factory function to allow LocalTime(hour, minute, second, [millis])
    local_time_class.as.class->factory = local_time_factory;
    
    // Store in globals
    do_set(vm->globals, "LocalTime", &local_time_class, sizeof(value_t));

    // Store a global reference for use in make_local_time
    static value_t local_time_class_storage;
    local_time_class_storage = vm_retain(local_time_class);
    global_local_time_class = &local_time_class_storage;
}