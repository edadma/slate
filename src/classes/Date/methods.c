#include "date_class.h"
#include "builtins.h"
#include "timezone.h"
#include "date.h"
#include "value.h"
#include "vm.h"
#include "zone.h"
#include "datetime.h"
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Helper macro to validate Date receiver
#define VALIDATE_DATE_RECEIVER(vm, args, arg_count, method_name) \
    do { \
        if (arg_count < 1) { \
            runtime_error(vm, method_name "() requires a receiver"); \
            return make_null(); \
        } \
        if (args[0].type != VAL_DATE) { \
            runtime_error(vm, method_name "() can only be called on Date objects"); \
            return make_null(); \
        } \
    } while(0)

// Date method: localDateTime()
value_t date_get_local_datetime_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.localDateTime");
    
    if (arg_count != 1) {
        runtime_error(vm, "localDateTime() takes no arguments");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    local_datetime_t* local_dt = date_get_local_datetime(date);
    
    return make_local_datetime(local_dt);
}

// Date method: zone()
value_t date_get_zone_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.zone");
    
    if (arg_count != 1) {
        runtime_error(vm, "zone() takes no arguments");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    const timezone_t* zone = date_get_zone(date);
    
    return make_zone_direct(zone);
}

// Date method: toInstant()
value_t date_to_instant_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.toInstant");
    
    if (arg_count != 1) {
        runtime_error(vm, "toInstant() takes no arguments");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int64_t epoch_millis = date_to_epoch_millis(date);
    
    return make_instant_direct(epoch_millis);
}

// Date method: withZone(zone) / atZone(zone)
value_t date_with_zone_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.withZone");
    
    if (arg_count != 2) {
        runtime_error(vm, "withZone() takes exactly 1 argument (Zone)");
        return make_null();
    }
    
    if (args[1].type != VAL_ZONE) {
        runtime_error(vm, "withZone() argument must be a Zone");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    const timezone_t* new_zone = args[1].as.zone;
    
    date_t* new_date = date_with_zone(vm, date, new_zone);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: withLocalDateTime(localDateTime)
value_t date_with_local_datetime_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.withLocalDateTime");
    
    if (arg_count != 2) {
        runtime_error(vm, "withLocalDateTime() takes exactly 1 argument (LocalDateTime)");
        return make_null();
    }
    
    if (args[1].type != VAL_LOCAL_DATETIME) {
        runtime_error(vm, "withLocalDateTime() argument must be a LocalDateTime");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    local_datetime_t* new_local_dt = args[1].as.local_datetime;
    
    date_t* new_date = date_with_local_datetime(vm, date, new_local_dt);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusHours(hours)
value_t date_plus_hours_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusHours");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusHours() takes exactly 1 argument (hours)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusHours() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int hours = value_to_int(args[1]);
    
    date_t* new_date = date_plus_hours(vm, date, hours);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusMinutes(minutes)
value_t date_plus_minutes_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusMinutes");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusMinutes() takes exactly 1 argument (minutes)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusMinutes() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int minutes = value_to_int(args[1]);
    
    date_t* new_date = date_plus_minutes(vm, date, minutes);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusSeconds(seconds)
value_t date_plus_seconds_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusSeconds");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusSeconds() takes exactly 1 argument (seconds)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusSeconds() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int seconds = value_to_int(args[1]);
    
    date_t* new_date = date_plus_seconds(vm, date, seconds);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusDays(days)
value_t date_plus_days_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusDays");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusDays() takes exactly 1 argument (days)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusDays() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int days = value_to_int(args[1]);
    
    date_t* new_date = date_plus_days(vm, date, days);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusMonths(months)
value_t date_plus_months_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusMonths");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusMonths() takes exactly 1 argument (months)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusMonths() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int months = value_to_int(args[1]);
    
    date_t* new_date = date_plus_months(vm, date, months);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: plusYears(years)
value_t date_plus_years_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.plusYears");
    
    if (arg_count != 2) {
        runtime_error(vm, "plusYears() takes exactly 1 argument (years)");
        return make_null();
    }
    
    if (!is_int(args[1])) {
        runtime_error(vm, "plusYears() argument must be an integer");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    int years = value_to_int(args[1]);
    
    date_t* new_date = date_plus_years(vm, date, years);
    if (!new_date) return make_null();
    
    return make_date_direct(new_date);
}

// Date method: isBefore(other)
value_t date_is_before_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.isBefore");
    
    if (arg_count != 2) {
        runtime_error(vm, "isBefore() takes exactly 1 argument (other Date)");
        return make_null();
    }
    
    if (args[1].type != VAL_DATE) {
        runtime_error(vm, "isBefore() argument must be a Date");
        return make_null();
    }
    
    date_t* this_date = args[0].as.date;
    date_t* other_date = args[1].as.date;
    
    bool result = date_is_before(this_date, other_date);
    return make_boolean(result);
}

// Date method: isAfter(other)
value_t date_is_after_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.isAfter");
    
    if (arg_count != 2) {
        runtime_error(vm, "isAfter() takes exactly 1 argument (other Date)");
        return make_null();
    }
    
    if (args[1].type != VAL_DATE) {
        runtime_error(vm, "isAfter() argument must be a Date");
        return make_null();
    }
    
    date_t* this_date = args[0].as.date;
    date_t* other_date = args[1].as.date;
    
    bool result = date_is_after(this_date, other_date);
    return make_boolean(result);
}

// Date method: equals(other)
value_t date_equals_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.equals");
    
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (other Date)");
        return make_null();
    }
    
    if (args[1].type != VAL_DATE) {
        return make_boolean(false);
    }
    
    date_t* this_date = args[0].as.date;
    date_t* other_date = args[1].as.date;
    
    bool result = date_equals(this_date, other_date);
    return make_boolean(result);
}

// Date method: toString()
value_t date_to_string_method(vm_t* vm, int arg_count, value_t* args) {
    VALIDATE_DATE_RECEIVER(vm, args, arg_count, "Date.toString");
    
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments");
        return make_null();
    }
    
    date_t* date = args[0].as.date;
    char* iso_string = date_to_iso_string(vm, date);
    if (!iso_string) return make_null();
    
    value_t result = make_string(iso_string);
    free(iso_string);
    return result;
}

// ============================================================================
// Date Core Implementation (moved from src/date.c)
// ============================================================================

// Date creation functions

date_t* date_create(vm_t* vm, local_datetime_t* local_dt, const timezone_t* zone) {
    if (!local_dt || !zone) {
        runtime_error(vm, "Date creation requires valid LocalDateTime and timezone");
        return NULL;
    }
    
    date_t* date = malloc(sizeof(date_t));
    if (!date) {
        runtime_error(vm, "Memory allocation failed for Date");
        return NULL;
    }
    
    date->ref_count = 1;
    date->local_dt = local_datetime_retain(local_dt);
    date->zone = zone; // Zone is managed by timezone system, not reference-counted
    
    return date;
}

date_t* date_now(vm_t* vm) {
    const timezone_t* system_tz = timezone_system();
    return date_now_in_zone(vm, system_tz);
}

date_t* date_now_in_zone(vm_t* vm, const timezone_t* zone) {
    if (!zone) {
        runtime_error(vm, "Date.now() requires a valid timezone");
        return NULL;
    }
    
    // Get current UTC time
    struct timespec ts;
    clock_gettime(CLOCK_REALTIME, &ts);
    int64_t epoch_millis = (int64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
    
    // Convert to the specified timezone
    return date_from_instant(vm, epoch_millis, zone);
}

date_t* date_of(vm_t* vm, int year, int month, int day, int hour, int minute, int second, const timezone_t* zone) {
    if (!zone) {
        runtime_error(vm, "Date creation requires a valid timezone");
        return NULL;
    }
    
    // Create local date and time components
    local_date_t* local_date = local_date_create(vm, year, month, day);
    if (!local_date) return NULL;
    
    local_time_t* local_time = local_time_create(vm, hour, minute, second, 0);
    if (!local_time) {
        local_date_release(local_date);
        return NULL;
    }
    
    local_datetime_t* local_dt = local_datetime_create(vm, local_date, local_time);
    if (!local_dt) {
        local_date_release(local_date);
        local_time_release(local_time);
        return NULL;
    }
    
    date_t* date = date_create(vm, local_dt, zone);
    
    // Release our references since date_create retains them
    local_date_release(local_date);
    local_time_release(local_time);
    local_datetime_release(local_dt);
    
    return date;
}

date_t* date_from_instant(vm_t* vm, int64_t epoch_millis, const timezone_t* zone) {
    if (!zone) {
        runtime_error(vm, "Date from Instant requires a valid timezone");
        return NULL;
    }
    
    // Get timezone offset for this time
    int16_t offset_minutes = timezone_get_offset(zone, epoch_millis);
    
    // Convert to local time by applying offset
    int64_t local_millis = epoch_millis + ((int64_t)offset_minutes * 60 * 1000);
    
    // Convert to local date/time components
    time_t local_seconds = local_millis / 1000;
    int millis = local_millis % 1000;
    
    struct tm local_tm;
    gmtime_r(&local_seconds, &local_tm);
    
    // Create Date components
    local_date_t* local_date = local_date_create(vm, 
        local_tm.tm_year + 1900, local_tm.tm_mon + 1, local_tm.tm_mday);
    if (!local_date) return NULL;
    
    local_time_t* local_time = local_time_create(vm,
        local_tm.tm_hour, local_tm.tm_min, local_tm.tm_sec, millis);
    if (!local_time) {
        local_date_release(local_date);
        return NULL;
    }
    
    local_datetime_t* local_dt = local_datetime_create(vm, local_date, local_time);
    if (!local_dt) {
        local_date_release(local_date);
        local_time_release(local_time);
        return NULL;
    }
    
    date_t* date = date_create(vm, local_dt, zone);
    
    // Release our references
    local_date_release(local_date);
    local_time_release(local_time);
    local_datetime_release(local_dt);
    
    return date;
}

// Memory management functions

date_t* date_retain(date_t* date) {
    if (date) {
        date->ref_count++;
    }
    return date;
}

void date_release(date_t* date) {
    if (date && --date->ref_count == 0) {
        local_datetime_release(date->local_dt);
        // Note: timezone is not released as it's managed by timezone system
        free(date);
    }
}

// Accessor functions

local_datetime_t* date_get_local_datetime(const date_t* date) {
    return date ? date->local_dt : NULL;
}

const timezone_t* date_get_zone(const date_t* date) {
    return date ? date->zone : NULL;
}

int64_t date_to_epoch_millis(const date_t* date) {
    if (!date || !date->local_dt) return 0;
    
    // Convert local datetime to epoch day/time
    local_date_t* ld = date->local_dt->date;
    local_time_t* lt = date->local_dt->time;
    
    // Get epoch day
    uint32_t epoch_day = ld->epoch_day;
    
    // Convert to milliseconds since epoch
    int64_t epoch_millis = ((int64_t)epoch_day * 24 * 60 * 60 * 1000) + 
                          ((int64_t)lt->hour * 60 * 60 * 1000) +
                          ((int64_t)lt->minute * 60 * 1000) +
                          ((int64_t)lt->second * 1000) +
                          lt->millis;
    
    // Subtract timezone offset to get UTC time
    int16_t offset_minutes = timezone_get_offset(date->zone, epoch_millis);
    return epoch_millis - ((int64_t)offset_minutes * 60 * 1000);
}

int16_t date_get_offset_minutes(const date_t* date) {
    if (!date) return 0;
    
    int64_t epoch_millis = date_to_epoch_millis(date);
    return timezone_get_offset(date->zone, epoch_millis);
}

bool date_is_dst(const date_t* date) {
    if (!date) return false;
    
    int64_t epoch_millis = date_to_epoch_millis(date);
    return timezone_is_dst(date->zone, epoch_millis);
}

// Conversion functions

date_t* date_with_zone(vm_t* vm, const date_t* date, const timezone_t* new_zone) {
    if (!date || !new_zone) {
        runtime_error(vm, "Date timezone conversion requires valid Date and timezone");
        return NULL;
    }
    
    // Convert to instant, then to new timezone
    int64_t epoch_millis = date_to_epoch_millis(date);
    return date_from_instant(vm, epoch_millis, new_zone);
}

date_t* date_with_local_datetime(vm_t* vm, const date_t* date, local_datetime_t* new_local_dt) {
    if (!date || !new_local_dt) {
        runtime_error(vm, "Date LocalDateTime replacement requires valid Date and LocalDateTime");
        return NULL;
    }
    
    return date_create(vm, new_local_dt, date->zone);
}

// Comparison functions

int date_compare(const date_t* a, const date_t* b) {
    if (!a || !b) {
        return (a != NULL) - (b != NULL);
    }
    
    // Compare as instants (UTC times)
    int64_t epoch_a = date_to_epoch_millis(a);
    int64_t epoch_b = date_to_epoch_millis(b);
    
    if (epoch_a < epoch_b) return -1;
    if (epoch_a > epoch_b) return 1;
    return 0;
}

bool date_equals(const date_t* a, const date_t* b) {
    return date_compare(a, b) == 0;
}

bool date_is_before(const date_t* a, const date_t* b) {
    return date_compare(a, b) < 0;
}

bool date_is_after(const date_t* a, const date_t* b) {
    return date_compare(a, b) > 0;
}

// Date arithmetic functions

date_t* date_plus_hours(vm_t* vm, const date_t* date, int hours) {
    if (!date) {
        runtime_error(vm, "Date arithmetic requires a valid Date");
        return NULL;
    }
    
    // Add hours and handle potential timezone transitions (DST changes)
    int64_t epoch_millis = date_to_epoch_millis(date);
    int64_t new_epoch_millis = epoch_millis + ((int64_t)hours * 60 * 60 * 1000);
    
    return date_from_instant(vm, new_epoch_millis, date->zone);
}

date_t* date_plus_minutes(vm_t* vm, const date_t* date, int minutes) {
    if (!date) {
        runtime_error(vm, "Date arithmetic requires a valid Date");
        return NULL;
    }
    
    int64_t epoch_millis = date_to_epoch_millis(date);
    int64_t new_epoch_millis = epoch_millis + ((int64_t)minutes * 60 * 1000);
    
    return date_from_instant(vm, new_epoch_millis, date->zone);
}

date_t* date_plus_seconds(vm_t* vm, const date_t* date, int seconds) {
    if (!date) {
        runtime_error(vm, "Date arithmetic requires a valid Date");
        return NULL;
    }
    
    int64_t epoch_millis = date_to_epoch_millis(date);
    int64_t new_epoch_millis = epoch_millis + ((int64_t)seconds * 1000);
    
    return date_from_instant(vm, new_epoch_millis, date->zone);
}

date_t* date_plus_days(vm_t* vm, const date_t* date, int days) {
    if (!date) {
        runtime_error(vm, "Date arithmetic requires a valid Date");
        return NULL;
    }
    
    int64_t epoch_millis = date_to_epoch_millis(date);
    int64_t new_epoch_millis = epoch_millis + ((int64_t)days * 24 * 60 * 60 * 1000);
    
    return date_from_instant(vm, new_epoch_millis, date->zone);
}

date_t* date_plus_months(vm_t* vm, const date_t* date, int months) {
    if (!date) {
        runtime_error(vm, "Date arithmetic requires a valid Date");
        return NULL;
    }
    
    // For month arithmetic, work with local date components
    local_date_t* new_local_date = local_date_plus_months(vm, date->local_dt->date, months);
    if (!new_local_date) return NULL;
    
    local_datetime_t* new_local_dt = local_datetime_create(vm, new_local_date, date->local_dt->time);
    if (!new_local_dt) {
        local_date_release(new_local_date);
        return NULL;
    }
    
    date_t* result = date_create(vm, new_local_dt, date->zone);
    
    local_date_release(new_local_date);
    local_datetime_release(new_local_dt);
    
    return result;
}

date_t* date_plus_years(vm_t* vm, const date_t* date, int years) {
    return date_plus_months(vm, date, years * 12);
}

// String conversion functions

char* date_to_string(vm_t* vm, const date_t* date) {
    if (!date) {
        runtime_error(vm, "Date string conversion requires a valid Date");
        return NULL;
    }
    
    return date_to_iso_string(vm, date);
}

char* date_to_iso_string(vm_t* vm, const date_t* date) {
    if (!date || !date->local_dt) {
        runtime_error(vm, "Date ISO string conversion requires a valid Date");
        return NULL;
    }
    
    // Get local datetime string
    char* local_str = local_datetime_to_string(vm, date->local_dt);
    if (!local_str) return NULL;
    
    // Get timezone offset
    int16_t offset_minutes = date_get_offset_minutes(date);
    
    // Format timezone offset (e.g., "-05:00", "+00:00")
    char offset_str[8]; // "+HH:MM\0" (need 7 chars + null terminator)
    int hours = abs(offset_minutes) / 60;
    int minutes = abs(offset_minutes) % 60;
    snprintf(offset_str, sizeof(offset_str), "%c%02d:%02d",
             offset_minutes < 0 ? '-' : '+', hours, minutes);
    
    // Get timezone ID for display
    const char* zone_id = timezone_get_id(date->zone);
    
    // Calculate result size: local_str + offset + [zone_id] + null
    size_t result_size = strlen(local_str) + strlen(offset_str) + strlen(zone_id) + 4; // +3 for "[" "]" and null
    
    char* result = malloc(result_size);
    if (!result) {
        free(local_str);
        runtime_error(vm, "Memory allocation failed for Date ISO string");
        return NULL;
    }
    
    snprintf(result, result_size, "%s%s[%s]", local_str, offset_str, zone_id);
    
    free(local_str);
    return result;
}