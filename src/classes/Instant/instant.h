#ifndef SLATE_INSTANT_H
#define SLATE_INSTANT_H

#include "vm.h"

// Instant factory functions (implemented in factory.c)
value_t instant_factory(vm_t* vm, class_t* self, int arg_count, value_t* args);
value_t instant_now(vm_t* vm, int arg_count, value_t* args);
value_t instant_of_epoch_second(vm_t* vm, int arg_count, value_t* args);
value_t instant_parse(vm_t* vm, int arg_count, value_t* args);

// Instant instance methods (implemented in methods.c)
value_t instant_to_epoch_milli(vm_t* vm, int arg_count, value_t* args);
value_t instant_to_epoch_second(vm_t* vm, int arg_count, value_t* args);
value_t instant_plus_millis(vm_t* vm, int arg_count, value_t* args);
value_t instant_minus_millis(vm_t* vm, int arg_count, value_t* args);
value_t instant_plus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t instant_minus_seconds(vm_t* vm, int arg_count, value_t* args);
value_t instant_is_before(vm_t* vm, int arg_count, value_t* args);
value_t instant_is_after(vm_t* vm, int arg_count, value_t* args);
value_t instant_equals(vm_t* vm, int arg_count, value_t* args);
value_t instant_to_string(vm_t* vm, int arg_count, value_t* args);

// Class registration function (implemented in class.c)
void init_instant_class(vm_t* vm);

// Utility functions
value_t make_instant_direct(int64_t epoch_millis);
value_t make_instant_direct_with_debug(int64_t epoch_millis, debug_location* debug);

#endif // SLATE_INSTANT_H