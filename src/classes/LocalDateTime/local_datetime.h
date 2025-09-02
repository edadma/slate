#ifndef CLASS_LOCAL_DATETIME_H
#define CLASS_LOCAL_DATETIME_H

#include "vm.h"
#include "value.h"

// LocalDateTime Class Initialization
void local_datetime_class_init(vm_t* vm);

// LocalDateTime Factory Function
value_t local_datetime_factory(vm_t* vm, int arg_count, value_t* args);

// LocalDateTime Instance Methods
value_t builtin_local_datetime_date(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_time(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_year(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_month(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_day(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_hour(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minute(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_second(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_millisecond(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_days(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_months(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_years(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_hours(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_minutes(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_plus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_days(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_months(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_years(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_hours(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_minutes(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_minus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_equals(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_is_before(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_is_after(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_datetime_to_string(vm_t* vm, int arg_count, value_t* args);

// LocalDateTime Static Methods
value_t builtin_local_datetime_now(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_LOCAL_DATETIME_H