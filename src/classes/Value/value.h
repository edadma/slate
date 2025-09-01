#ifndef SLATE_VALUE_CLASS_H
#define SLATE_VALUE_CLASS_H

// Forward declarations
typedef struct slate_vm slate_vm;
typedef struct value value_t;

// Value Utility Functions
value_t builtin_type(slate_vm* vm, int arg_count, value_t* args);
value_t builtin_value_to_string(slate_vm* vm, int arg_count, value_t* args);

#endif // SLATE_VALUE_CLASS_H