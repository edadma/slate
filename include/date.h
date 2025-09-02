#ifndef SLATE_DATE_H
#define SLATE_DATE_H

#include "datetime.h"
#include "timezone.h"
#include <stdint.h>

// Date structure: combines LocalDateTime with timezone information
// This is the primary zoned datetime type in Slate
struct date {
    int ref_count;                  // Reference counting for memory management
    local_datetime_t* local_dt;     // Local date and time components
    const timezone_t* zone;         // Timezone information
};

// Memory management functions
date_t* date_create(vm_t* vm, local_datetime_t* local_dt, const timezone_t* zone);
date_t* date_now(vm_t* vm);
date_t* date_now_in_zone(vm_t* vm, const timezone_t* zone);
date_t* date_of(vm_t* vm, int year, int month, int day, int hour, int minute, int second, const timezone_t* zone);
date_t* date_from_instant(vm_t* vm, int64_t epoch_millis, const timezone_t* zone);

date_t* date_retain(date_t* date);
void date_release(date_t* date);

// Accessor functions
local_datetime_t* date_get_local_datetime(const date_t* date);
const timezone_t* date_get_zone(const date_t* date);
int64_t date_to_epoch_millis(const date_t* date);
int16_t date_get_offset_minutes(const date_t* date);
bool date_is_dst(const date_t* date);

// Conversion functions
date_t* date_with_zone(vm_t* vm, const date_t* date, const timezone_t* new_zone);
date_t* date_with_local_datetime(vm_t* vm, const date_t* date, local_datetime_t* new_local_dt);

// Comparison functions
int date_compare(const date_t* a, const date_t* b);
bool date_equals(const date_t* a, const date_t* b);
bool date_is_before(const date_t* a, const date_t* b);
bool date_is_after(const date_t* a, const date_t* b);

// Date arithmetic (returns new Date instances)
date_t* date_plus_hours(vm_t* vm, const date_t* date, int hours);
date_t* date_plus_minutes(vm_t* vm, const date_t* date, int minutes);
date_t* date_plus_seconds(vm_t* vm, const date_t* date, int seconds);
date_t* date_plus_days(vm_t* vm, const date_t* date, int days);
date_t* date_plus_months(vm_t* vm, const date_t* date, int months);
date_t* date_plus_years(vm_t* vm, const date_t* date, int years);

// String conversion
char* date_to_string(vm_t* vm, const date_t* date);
char* date_to_iso_string(vm_t* vm, const date_t* date);

#endif // SLATE_DATE_H