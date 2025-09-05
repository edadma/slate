#include "date_class.h"
#include "builtins.h"
#include "timezone.h"
#include "date.h"
#include "zone.h"
#include "value.h"
#include "vm.h"

// Date factory function (constructor)
value_t date_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    runtime_error(vm, "Date() constructor is not available. Use Date.now(), Date.of(), or Date.fromInstant()");
    return make_null();
}

// Date.now() - Get current date/time in system timezone
value_t date_now_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "Date.now() takes no arguments");
        return make_null();
    }
    
    date_t* date_obj = date_now(vm);
    if (!date_obj) return make_null();
    
    return make_date_direct(date_obj);
}

// Date.nowInZone(zone) - Get current date/time in specified timezone
value_t date_now_in_zone_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "Date.nowInZone() takes exactly 1 argument (Zone)");
        return make_null();
    }
    
    if (args[0].type != VAL_ZONE) {
        runtime_error(vm, "Date.nowInZone() argument must be a Zone");
        return make_null();
    }
    
    const timezone_t* zone = args[0].as.zone;
    date_t* date_obj = date_now_in_zone(vm, zone);
    if (!date_obj) return make_null();
    
    return make_date_direct(date_obj);
}

// Date.of(year, month, day, hour, minute, second, zone)
value_t date_of_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 7) {
        runtime_error(vm, "Date.of() requires 7 arguments: year, month, day, hour, minute, second, zone");
        return make_null();
    }
    
    // Validate first 6 arguments are numbers
    for (int i = 0; i < 6; i++) {
        if (!is_int(args[i])) {
            runtime_error(vm, "Date.of() arguments 1-6 must be integers");
            return make_null();
        }
    }
    
    // Validate 7th argument is a Zone
    if (args[6].type != VAL_ZONE) {
        runtime_error(vm, "Date.of() argument 7 must be a Zone");
        return make_null();
    }
    
    int year = value_to_int(args[0]);
    int month = value_to_int(args[1]);
    int day = value_to_int(args[2]);
    int hour = value_to_int(args[3]);
    int minute = value_to_int(args[4]);
    int second = value_to_int(args[5]);
    const timezone_t* zone = args[6].as.zone;
    
    date_t* date = date_of(vm, year, month, day, hour, minute, second, zone);
    if (!date) return make_null();
    
    return make_date_direct(date);
}

// Date.fromInstant(instant, zone)
value_t date_from_instant_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "Date.fromInstant() takes exactly 2 arguments (Instant, Zone)");
        return make_null();
    }
    
    if (args[0].type != VAL_INSTANT) {
        runtime_error(vm, "Date.fromInstant() first argument must be an Instant");
        return make_null();
    }
    
    if (args[1].type != VAL_ZONE) {
        runtime_error(vm, "Date.fromInstant() second argument must be a Zone");
        return make_null();
    }
    
    int64_t epoch_millis = args[0].as.instant_millis;
    const timezone_t* zone = args[1].as.zone;
    
    date_t* date = date_from_instant(vm, epoch_millis, zone);
    if (!date) return make_null();
    
    return make_date_direct(date);
}

// Date.parse(iso_string) - Parse ISO 8601 date string
value_t date_parse(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "Date.parse() takes exactly 1 argument (ISO date string)");
        return make_null();
    }
    
    if (args[0].type != VAL_STRING) {
        runtime_error(vm, "Date.parse() argument must be a string");
        return make_null();
    }
    
    const char* iso_string = args[0].as.string;
    
    // TODO: Implement ISO 8601 parsing
    // For now, just return an error
    runtime_error(vm, "Date.parse() is not yet implemented");
    return make_null();
}