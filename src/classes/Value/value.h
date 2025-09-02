#ifndef CLASS_VALUE_H
#define CLASS_VALUE_H

#include "vm.h"
#include "value.h"

// Value Utility Functions
value_t builtin_type(vm_t* vm, int arg_count, value_t* args);
value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args);

#endif // CLASS_VALUE_H