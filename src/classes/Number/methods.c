#include "number.h"
#include "builtins.h"
#include "dynamic_int.h"
#include <math.h>

// Number method: min(other) - Returns the smaller of two numeric values
value_t builtin_number_min(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "min() requires exactly 1 argument");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    // Check that both values are numeric
    if (!is_number(receiver)) {
        runtime_error(vm, "min() can only be called on numbers");
        return make_null();
    }
    
    if (!is_number(other)) {
        runtime_error(vm, "min() argument must be a number");
        return make_null();
    }
    
    // Use the generalized comparison function
    if (compare_numbers(receiver, other) < 0) {
        return vm_retain(receiver);
    } else {
        return vm_retain(other);
    }
}

// Number method: max(other) - Returns the larger of two numeric values
value_t builtin_number_max(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "max() requires exactly 1 argument");
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    // Check that both values are numeric
    if (!is_number(receiver)) {
        runtime_error(vm, "max() can only be called on numbers");
        return make_null();
    }
    
    if (!is_number(other)) {
        runtime_error(vm, "max() argument must be a number");
        return make_null();
    }
    
    // Use the generalized comparison function
    if (compare_numbers(receiver, other) > 0) {
        return vm_retain(receiver);
    } else {
        return vm_retain(other);
    }
}