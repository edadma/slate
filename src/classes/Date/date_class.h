#ifndef SLATE_DATE_CLASS_H
#define SLATE_DATE_CLASS_H

#include "vm.h"
#include "value.h"
#include "date.h"

// Date factory functions (implemented in factory.c)
value_t date_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t date_now_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t date_now_in_zone_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t date_of_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t date_from_instant_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t date_parse(vm_t* vm, int arg_count, value_t* args);

// Date instance methods (implemented in methods.c)
value_t date_get_local_datetime_method(vm_t* vm, int arg_count, value_t* args);
value_t date_get_zone_method(vm_t* vm, int arg_count, value_t* args);
value_t date_to_instant_method(vm_t* vm, int arg_count, value_t* args);
value_t date_with_zone_method(vm_t* vm, int arg_count, value_t* args);
value_t date_with_local_datetime_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_hours_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_minutes_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_seconds_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_days_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_months_method(vm_t* vm, int arg_count, value_t* args);
value_t date_plus_years_method(vm_t* vm, int arg_count, value_t* args);
value_t date_is_before_method(vm_t* vm, int arg_count, value_t* args);
value_t date_is_after_method(vm_t* vm, int arg_count, value_t* args);
value_t date_equals_method(vm_t* vm, int arg_count, value_t* args);
value_t date_to_string_method(vm_t* vm, int arg_count, value_t* args);

// Class registration function (implemented in class.c)
void init_date_class(vm_t* vm);

// Utility functions
value_t make_date_direct(date_t* date);
value_t make_date_direct_with_debug(date_t* date, debug_location* debug);

#endif // SLATE_DATE_CLASS_H