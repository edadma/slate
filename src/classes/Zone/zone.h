#ifndef SLATE_ZONE_H
#define SLATE_ZONE_H

#include "vm.h"
#include "value.h"
#include "timezone.h"

// Zone factory functions (implemented in factory.c)
value_t zone_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t zone_utc(vm_t* vm, int arg_count, value_t* args);
value_t zone_system(vm_t* vm, int arg_count, value_t* args);
value_t zone_of(vm_t* vm, int arg_count, value_t* args);

// Zone instance methods (implemented in methods.c)
value_t zone_id(vm_t* vm, int arg_count, value_t* args);
value_t zone_offset(vm_t* vm, int arg_count, value_t* args);
value_t zone_is_dst(vm_t* vm, int arg_count, value_t* args);
value_t zone_display_name(vm_t* vm, int arg_count, value_t* args);
value_t zone_equals(vm_t* vm, int arg_count, value_t* args);
value_t zone_to_string(vm_t* vm, int arg_count, value_t* args);

// Class registration function (implemented in class.c)
void init_zone_class(vm_t* vm);

// Utility functions
value_t make_zone_direct(const timezone_t* timezone);
value_t make_zone_direct_with_debug(const timezone_t* timezone, debug_location* debug);

#endif // SLATE_ZONE_H