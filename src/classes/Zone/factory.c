#include "zone.h"
#include "builtins.h"
#include "timezone.h"
#include "value.h"
#include "vm.h"

// Zone factory function (constructor)
value_t zone_factory(vm_t* vm, int arg_count, value_t* args) {
    runtime_error(vm, "Zone() constructor is not available. Use Zone.of(), Zone.utc(), or Zone.system()");
    return make_null();
}

// Zone.utc() - Get UTC timezone
value_t zone_utc(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "Zone.utc() takes no arguments");
        return make_null();
    }
    
    const timezone_t* utc_tz = timezone_utc();
    return make_zone_direct(utc_tz);
}

// Zone.system() - Get system default timezone
value_t zone_system(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "Zone.system() takes no arguments");
        return make_null();
    }
    
    const timezone_t* system_tz = timezone_system();
    return make_zone_direct(system_tz);
}

// Zone.of(timezone_id) - Get timezone by ID
value_t zone_of(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "Zone.of() takes exactly 1 argument (timezone ID)");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING) {
        runtime_error(vm, "Zone.of() argument must be a string");
        return make_null();
    }
    
    const char* timezone_id = args[0].as.string;
    
    // Validate timezone ID
    if (!is_valid_timezone_id(timezone_id)) {
        runtime_error(vm, "Invalid timezone ID: %s", timezone_id);
        return make_null();
    }
    
    // Get timezone
    const timezone_t* tz = timezone_of(timezone_id);
    if (!tz) {
        runtime_error(vm, "Unknown timezone: %s", timezone_id);
        return make_null();
    }
    
    return make_zone_direct(tz);
}