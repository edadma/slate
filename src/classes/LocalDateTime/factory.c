#include "local_datetime.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "vm.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

// Forward declaration for ISO parsing
local_datetime_t* local_datetime_parse_iso(vm_t* vm, const char* iso_string);

// LocalDateTime factory function
value_t local_datetime_factory(vm_t* vm, int arg_count, value_t* args) {
    // Case 1: Single string argument (ISO 8601 format)
    if (arg_count == 1 && args[0].type == VAL_STRING) {
        const char* iso_string = args[0].as.string;
        local_datetime_t* dt = local_datetime_parse_iso(vm, iso_string);
        if (dt == NULL) {
            runtime_error(vm, "Invalid ISO 8601 datetime string");
        }
        return make_local_datetime(dt);
    }
    
    // Case 2: Two arguments (LocalDate and LocalTime)
    if (arg_count == 2) {
        if (args[0].type != VAL_LOCAL_DATE) {
            runtime_error(vm, "LocalDateTime() first argument must be a LocalDate");
        }
        if (args[1].type != VAL_LOCAL_TIME) {
            runtime_error(vm, "LocalDateTime() second argument must be a LocalTime");
        }
        
        local_date_t* date = args[0].as.local_date;
        local_time_t* time = args[1].as.local_time;
        
        local_datetime_t* dt = local_datetime_create(vm, date, time);
        return make_local_datetime(dt);
    }
    
    // Case 3: 6 or 7 arguments (year, month, day, hour, minute, second, [millisecond])
    if (arg_count == 6 || arg_count == 7) {
        // Validate all arguments are integers
        for (int i = 0; i < arg_count; i++) {
            if (!is_int(args[i])) {
                runtime_error(vm, "LocalDateTime() components must be integers");
            }
        }
        
        int year = value_to_int(args[0]);
        int month = value_to_int(args[1]);
        int day = value_to_int(args[2]);
        int hour = value_to_int(args[3]);
        int minute = value_to_int(args[4]);
        int second = value_to_int(args[5]);
        int millis = (arg_count == 7) ? value_to_int(args[6]) : 0;
        
        // Validate date and time components
        if (!is_valid_date(year, month, day)) {
            runtime_error(vm, "Invalid date parameters");
        }
        if (!is_valid_time(hour, minute, second, millis)) {
            runtime_error(vm, "Invalid time parameters");
        }
        
        // Create LocalDate and LocalTime components
        local_date_t* date = local_date_create(vm, year, month, day);
        local_time_t* time = local_time_create(vm, hour, minute, second, millis);
        
        // Create LocalDateTime
        local_datetime_t* dt = local_datetime_create(vm, date, time);
        
        // Release our references since local_datetime_create retains them
        local_date_release(date);
        local_time_release(time);
        
        return make_local_datetime(dt);
    }
    
    runtime_error(vm, "LocalDateTime() wrong number of arguments");
}

// Parse ISO 8601 datetime string
// Supports formats like:
// - "2024-12-25T15:30:45"
// - "2024-12-25T15:30:45.123"
// - "2024-12-25 15:30:45"
// - "2024-12-25 15:30:45.123"
local_datetime_t* local_datetime_parse_iso(vm_t* vm, const char* iso_string) {
    if (iso_string == NULL || strlen(iso_string) < 19) {
        return NULL; // Too short to be valid
    }
    
    // Make a copy for parsing
    char buffer[30];
    strncpy(buffer, iso_string, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';
    
    // Parse components
    int year, month, day, hour, minute, second, millis = 0;
    char separator;
    
    // Try parsing with milliseconds first
    int parsed = sscanf(buffer, "%4d-%2d-%2d%c%2d:%2d:%2d.%3d",
                        &year, &month, &day, &separator, &hour, &minute, &second, &millis);
    
    if (parsed < 7) {
        // Try without milliseconds
        millis = 0;
        parsed = sscanf(buffer, "%4d-%2d-%2d%c%2d:%2d:%2d",
                       &year, &month, &day, &separator, &hour, &minute, &second);
        
        if (parsed != 7) {
            return NULL; // Invalid format
        }
    }
    
    // Validate separator (T or space)
    if (separator != 'T' && separator != ' ') {
        return NULL;
    }
    
    // Validate components
    if (!is_valid_date(year, month, day) || !is_valid_time(hour, minute, second, millis)) {
        return NULL;
    }
    
    // Create LocalDate and LocalTime
    local_date_t* date = local_date_create(vm, year, month, day);
    local_time_t* time = local_time_create(vm, hour, minute, second, millis);
    
    // Create LocalDateTime
    local_datetime_t* dt = local_datetime_create(vm, date, time);
    
    // Release our references
    local_date_release(date);
    local_time_release(time);
    
    return dt;
}