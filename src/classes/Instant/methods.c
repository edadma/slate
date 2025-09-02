#include "instant.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "vm.h"
#include <stdio.h>
#include <time.h>
#include <string.h>

// Helper macro to validate Instant receiver
#define VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, method_name) \
    do { \
        if (arg_count < 1) { \
            runtime_error(vm, method_name "() requires a receiver"); \
            return make_null(); \
        } \
        if (args[0].type != VAL_INSTANT) { \
            runtime_error(vm, method_name "() can only be called on Instant objects"); \
            return make_null(); \
        } \
    } while(0)

// Instant method: toEpochMilli()
value_t instant_to_epoch_milli(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.toEpochMilli");
    
    if (arg_count != 1) {
        runtime_error(vm, "toEpochMilli() takes no arguments");
        return make_null();
    }
    
    int64_t epoch_millis = args[0].as.instant_millis;
    // Convert int64_t to bigint for large epoch times
    di_int bigint_millis = di_from_int64(epoch_millis);
    return make_bigint(bigint_millis);
}

// Instant method: toEpochSecond()  
value_t instant_to_epoch_second(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.toEpochSecond");
    
    if (arg_count != 1) {
        runtime_error(vm, "toEpochSecond() takes no arguments");
        return make_null();
    }
    
    int64_t epoch_millis = args[0].as.instant_millis;
    int64_t epoch_seconds = epoch_millis / 1000;
    // Convert int64_t to bigint for large epoch times
    di_int bigint_seconds = di_from_int64(epoch_seconds);
    return make_bigint(bigint_seconds);
}

// Instant method: plusMillis(millis)
value_t instant_plus_millis(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.plusMillis");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusMillis() takes exactly 1 argument (milliseconds)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusMillis() argument must be an integer");
        return make_null();
    }
    
    int64_t current_millis = args[0].as.instant_millis;
    int64_t add_millis = value_to_int(args[1]);
    int64_t result_millis = current_millis + add_millis;
    
    // Check for overflow
    if ((add_millis > 0 && result_millis < current_millis) ||
        (add_millis < 0 && result_millis > current_millis)) {
        runtime_error(vm, "plusMillis() operation causes overflow");
        return make_null();
    }
    
    return make_instant_direct(result_millis);
}

// Instant method: minusMillis(millis)
value_t instant_minus_millis(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.minusMillis");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusMillis() takes exactly 1 argument (milliseconds)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusMillis() argument must be an integer");
        return make_null();
    }
    
    int64_t current_millis = args[0].as.instant_millis;
    int64_t sub_millis = value_to_int(args[1]);
    int64_t result_millis = current_millis - sub_millis;
    
    // Check for overflow/underflow
    if ((sub_millis > 0 && result_millis > current_millis) ||
        (sub_millis < 0 && result_millis < current_millis)) {
        runtime_error(vm, "minusMillis() operation causes overflow");
        return make_null();
    }
    
    return make_instant_direct(result_millis);
}

// Instant method: plusSeconds(seconds)
value_t instant_plus_seconds(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.plusSeconds");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusSeconds() takes exactly 1 argument (seconds)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusSeconds() argument must be an integer");
        return make_null();
    }
    
    int64_t current_millis = args[0].as.instant_millis;
    int64_t add_seconds = value_to_int(args[1]);
    
    // Check for overflow in multiplication
    if (add_seconds > INT64_MAX / 1000 || add_seconds < INT64_MIN / 1000) {
        runtime_error(vm, "plusSeconds() argument too large: %lld", add_seconds);
        return make_null();
    }
    
    int64_t add_millis = add_seconds * 1000;
    int64_t result_millis = current_millis + add_millis;
    
    // Check for overflow
    if ((add_millis > 0 && result_millis < current_millis) ||
        (add_millis < 0 && result_millis > current_millis)) {
        runtime_error(vm, "plusSeconds() operation causes overflow");
        return make_null();
    }
    
    return make_instant_direct(result_millis);
}

// Instant method: minusSeconds(seconds)
value_t instant_minus_seconds(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.minusSeconds");
    
    if (arg_count != 2) {
        runtime_error(vm, "minusSeconds() takes exactly 1 argument (seconds)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "minusSeconds() argument must be an integer");
        return make_null();
    }
    
    int64_t current_millis = args[0].as.instant_millis;
    int64_t sub_seconds = value_to_int(args[1]);
    
    // Check for overflow in multiplication
    if (sub_seconds > INT64_MAX / 1000 || sub_seconds < INT64_MIN / 1000) {
        runtime_error(vm, "minusSeconds() argument too large: %lld", sub_seconds);
        return make_null();
    }
    
    int64_t sub_millis = sub_seconds * 1000;
    int64_t result_millis = current_millis - sub_millis;
    
    // Check for overflow/underflow
    if ((sub_millis > 0 && result_millis > current_millis) ||
        (sub_millis < 0 && result_millis < current_millis)) {
        runtime_error(vm, "minusSeconds() operation causes overflow");
        return make_null();
    }
    
    return make_instant_direct(result_millis);
}

// Instant method: isBefore(other)
value_t instant_is_before(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.isBefore");
    
    if (arg_count != 2) {
        runtime_error(vm, "isBefore() takes exactly 1 argument (other Instant)");
        return make_null();
    }
    
    if (args[1].type != VAL_INSTANT) {
        runtime_error(vm, "isBefore() argument must be an Instant");
        return make_null();
    }
    
    int64_t this_millis = args[0].as.instant_millis;
    int64_t other_millis = args[1].as.instant_millis;
    
    return make_boolean(this_millis < other_millis);
}

// Instant method: isAfter(other)
value_t instant_is_after(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.isAfter");
    
    if (arg_count != 2) {
        runtime_error(vm, "isAfter() takes exactly 1 argument (other Instant)");
        return make_null();
    }
    
    if (args[1].type != VAL_INSTANT) {
        runtime_error(vm, "isAfter() argument must be an Instant");
        return make_null();
    }
    
    int64_t this_millis = args[0].as.instant_millis;
    int64_t other_millis = args[1].as.instant_millis;
    
    return make_boolean(this_millis > other_millis);
}

// Instant method: equals(other)
value_t instant_equals(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.equals");
    
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (other Instant)");
        return make_null();
    }
    
    if (args[1].type != VAL_INSTANT) {
        return make_boolean(false);
    }
    
    int64_t this_millis = args[0].as.instant_millis;
    int64_t other_millis = args[1].as.instant_millis;
    
    return make_boolean(this_millis == other_millis);
}

// Instant method: toString()
value_t instant_to_string(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_INSTANT_RECEIVER(vm, args, arg_count, "Instant.toString");
    
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments");
        return make_null();
    }
    
    int64_t epoch_millis = args[0].as.instant_millis;
    
    // Convert to seconds and milliseconds
    time_t epoch_seconds = (time_t)(epoch_millis / 1000);
    int millis = (int)(epoch_millis % 1000);
    if (millis < 0) {
        millis += 1000;
        epoch_seconds -= 1;
    }
    
    // Convert to UTC time
    struct tm* utc_tm = gmtime(&epoch_seconds);
    if (!utc_tm) {
        runtime_error(vm, "Failed to convert instant to UTC time");
        return make_null();
    }
    
    // Format as ISO 8601 string
    char iso_buffer[64];
    if (millis > 0) {
        snprintf(iso_buffer, sizeof(iso_buffer),
                "%04d-%02d-%02dT%02d:%02d:%02d.%03dZ",
                utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday,
                utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec, millis);
    } else {
        snprintf(iso_buffer, sizeof(iso_buffer),
                "%04d-%02d-%02dT%02d:%02d:%02dZ",
                utc_tm->tm_year + 1900, utc_tm->tm_mon + 1, utc_tm->tm_mday,
                utc_tm->tm_hour, utc_tm->tm_min, utc_tm->tm_sec);
    }
    
    return make_string(iso_buffer);
}