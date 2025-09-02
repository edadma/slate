#include "zone.h"
#include "builtins.h"
#include "timezone.h"
#include "value.h"
#include "vm.h"
#include <string.h>

// Helper macro to validate Zone receiver
#define VALIDATE_ZONE_RECEIVER(vm, args, arg_count, method_name) \
    do { \
        if (arg_count < 1) { \
            runtime_error(vm, method_name "() requires a receiver"); \
            return make_null(); \
        } \
        if (args[0].type != VAL_ZONE) { \
            runtime_error(vm, method_name "() can only be called on Zone objects"); \
            return make_null(); \
        } \
    } while(0)

// Zone method: id()
value_t zone_id(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.id");
    
    if (arg_count != 1) {
        runtime_error(vm, "id() takes no arguments");
        return make_null();
    }
    
    const timezone_t* timezone = args[0].as.zone;
    const char* id = timezone_get_id(timezone);
    
    return make_string(id);
}

// Zone method: offset(instant_or_epoch_millis)
value_t zone_offset(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.offset");
    
    if (arg_count != 2) {
        runtime_error(vm, "offset() takes exactly 1 argument (Instant or epoch milliseconds)");
        return make_null();
    }
    
    const timezone_t* timezone = args[0].as.zone;
    int64_t epoch_millis;
    
    // Accept either Instant or numeric epoch milliseconds
    if (args[1].type == VAL_INSTANT) {
        epoch_millis = args[1].as.instant_millis;
    } else if (is_int(args[1])) {
        epoch_millis = value_to_int(args[1]);
    } else {
        runtime_error(vm, "offset() argument must be an Instant or integer epoch milliseconds");
        return make_null();
    }
    
    // Get timezone offset in minutes
    int16_t offset_minutes = timezone_get_offset(timezone, epoch_millis);
    
    // Format as ISO 8601 offset string (e.g., "-05:00", "+00:00")
    char offset_str[8];
    int hours = abs(offset_minutes) / 60;
    int minutes = abs(offset_minutes) % 60;
    
    snprintf(offset_str, sizeof(offset_str), "%c%02d:%02d",
             offset_minutes < 0 ? '-' : '+', hours, minutes);
    
    return make_string(offset_str);
}

// Zone method: isDst(instant_or_epoch_millis)
value_t zone_is_dst(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.isDst");
    
    if (arg_count != 2) {
        runtime_error(vm, "isDst() takes exactly 1 argument (Instant or epoch milliseconds)");
        return make_null();
    }
    
    const timezone_t* timezone = args[0].as.zone;
    int64_t epoch_millis;
    
    // Accept either Instant or numeric epoch milliseconds
    if (args[1].type == VAL_INSTANT) {
        epoch_millis = args[1].as.instant_millis;
    } else if (is_int(args[1])) {
        epoch_millis = value_to_int(args[1]);
    } else {
        runtime_error(vm, "isDst() argument must be an Instant or integer epoch milliseconds");
        return make_null();
    }
    
    bool is_dst = timezone_is_dst(timezone, epoch_millis);
    return make_boolean(is_dst);
}

// Zone method: displayName()
value_t zone_display_name(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.displayName");
    
    // Accept optional DST parameter
    if (arg_count < 1 || arg_count > 2) {
        runtime_error(vm, "displayName() takes 0 or 1 arguments (optional isDst boolean)");
        return make_null();
    }
    
    const timezone_t* timezone = args[0].as.zone;
    bool dst = false;
    
    if (arg_count == 2) {
        if (args[1].type != VAL_BOOLEAN) {
            runtime_error(vm, "displayName() isDst argument must be a boolean");
            return make_null();
        }
        dst = args[1].as.boolean;
    }
    
    const char* display_name = timezone_get_display_name(timezone, dst);
    return make_string(display_name);
}

// Zone method: equals(other)
value_t zone_equals(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.equals");
    
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (other Zone)");
        return make_null();
    }
    
    if (args[1].type != VAL_ZONE) {
        return make_boolean(false);
    }
    
    const timezone_t* this_timezone = args[0].as.zone;
    const timezone_t* other_timezone = args[1].as.zone;
    
    // Compare timezone IDs
    const char* this_id = timezone_get_id(this_timezone);
    const char* other_id = timezone_get_id(other_timezone);
    
    bool equal = strcmp(this_id, other_id) == 0;
    return make_boolean(equal);
}

// Zone method: toString()
value_t zone_to_string(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_ZONE_RECEIVER(vm, args, arg_count, "Zone.toString");
    
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments");
        return make_null();
    }
    
    const timezone_t* timezone = args[0].as.zone;
    const char* id = timezone_get_id(timezone);
    
    return make_string(id);
}