#include "instant.h"
#include "builtins.h"
#include "datetime.h"
#include "value.h"
#include "vm.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// Forward declaration for ISO parsing
int64_t instant_parse_iso(const char* iso_string);

// Main Instant factory function
value_t instant_factory(vm_t* vm, int arg_count, value_t* args) {
    // Case 1: Single integer argument (epoch milliseconds)
    if (arg_count == 1) {
        if (is_int(args[0])) {
            int64_t epoch_millis = value_to_int(args[0]);
            return make_instant_direct(epoch_millis);
        } else if (args[0].type == VAL_STRING) {
            // Parse ISO 8601 string
            const char* iso_string = args[0].as.string;
            int64_t epoch_millis = instant_parse_iso(iso_string);
            if (epoch_millis == INT64_MIN) {
                runtime_error(vm, "Invalid ISO 8601 instant string: %s", iso_string);
            }
            return make_instant_direct(epoch_millis);
        } else {
            runtime_error(vm, "Instant() argument must be an integer (epoch milliseconds) or string (ISO 8601)");
        }
    }
    
    // Case 2: No arguments (equivalent to Instant.now())
    if (arg_count == 0) {
        return instant_now(vm, 0, NULL);
    }
    
    runtime_error(vm, "Instant() takes 0 or 1 argument, got %d", arg_count);
    return make_null();
}

// Instant.now() - Current system time in milliseconds
value_t instant_now(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 0) {
        runtime_error(vm, "Instant.now() takes no arguments");
    }
    
    // Get current time using system clock
    struct timespec ts;
    if (clock_gettime(CLOCK_REALTIME, &ts) != 0) {
        runtime_error(vm, "Failed to get current time");
    }
    
    // Convert to milliseconds since epoch
    int64_t epoch_millis = (int64_t)ts.tv_sec * 1000 + (int64_t)ts.tv_nsec / 1000000;
    return make_instant_direct(epoch_millis);
}

// Instant.ofEpochSecond() - Create from epoch seconds
value_t instant_of_epoch_second(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "Instant.ofEpochSecond() takes exactly 1 argument (seconds)");
    }
    
    if (!is_int(args[0])) {
        runtime_error(vm, "Instant.ofEpochSecond() argument must be an integer");
    }
    
    int64_t epoch_seconds = value_to_int(args[0]);
    int64_t epoch_millis = epoch_seconds * 1000;
    
    // Check for overflow
    if (epoch_seconds > 0 && epoch_millis < 0) {
        runtime_error(vm, "Epoch seconds value causes overflow: %lld", epoch_seconds);
    }
    
    return make_instant_direct(epoch_millis);
}

// Instant.parse() - Parse ISO 8601 string
value_t instant_parse(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "Instant.parse() takes exactly 1 argument (ISO string)");
    }
    
    if (args[0].type != VAL_STRING) {
        runtime_error(vm, "Instant.parse() argument must be a string");
    }
    
    const char* iso_string = args[0].as.string;
    int64_t epoch_millis = instant_parse_iso(iso_string);
    if (epoch_millis == INT64_MIN) {
        runtime_error(vm, "Invalid ISO 8601 instant string: %s", iso_string);
    }
    
    return make_instant_direct(epoch_millis);
}

// Helper function to parse ISO 8601 instant strings
// Returns INT64_MIN on error
int64_t instant_parse_iso(const char* iso_string) {
    if (!iso_string) return INT64_MIN;
    
    // Basic ISO 8601 parsing - supports formats like:
    // "2024-01-15T10:30:45Z"
    // "2024-01-15T10:30:45.123Z"
    // "2024-01-15T10:30:45+00:00"
    
    struct tm tm = {0};
    int millis = 0;
    int parsed = 0;
    
    // Try parsing with milliseconds first
    parsed = sscanf(iso_string, "%d-%d-%dT%d:%d:%d.%dZ",
                   &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                   &tm.tm_hour, &tm.tm_min, &tm.tm_sec, &millis);
    
    if (parsed == 7) {
        // Successfully parsed with milliseconds
        tm.tm_year -= 1900; // Adjust for tm_year (years since 1900)
        tm.tm_mon -= 1;     // Adjust for tm_mon (0-based months)
    } else {
        // Try parsing without milliseconds
        millis = 0;
        parsed = sscanf(iso_string, "%d-%d-%dT%d:%d:%dZ",
                       &tm.tm_year, &tm.tm_mon, &tm.tm_mday,
                       &tm.tm_hour, &tm.tm_min, &tm.tm_sec);
        
        if (parsed != 6) {
            return INT64_MIN; // Parse error
        }
        
        tm.tm_year -= 1900;
        tm.tm_mon -= 1;
    }
    
    // Convert to UTC timestamp
    time_t epoch_seconds = timegm(&tm);
    if (epoch_seconds == -1) {
        return INT64_MIN;
    }
    
    // Convert to milliseconds and add millisecond component
    return (int64_t)epoch_seconds * 1000 + millis;
}