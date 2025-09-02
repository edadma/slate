#include "date_class.h"
#include "builtins.h"
#include "timezone.h"
#include "date.h"
#include "value.h"
#include "vm.h"
#include "zone.h"
#include <string.h>

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