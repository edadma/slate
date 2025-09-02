#ifndef SLATE_VALUE_CLASS_H
#define SLATE_VALUE_CLASS_H

// Forward declarations
typedef struct slate_vm vm_t;
typedef struct value value_t;

// Value Utility Functions
value_t builtin_type(vm_t* vm, int arg_count, value_t* args);
value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args);

#endif // SLATE_VALUE_CLASS_H