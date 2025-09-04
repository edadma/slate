#include "class_null.h"
#include "builtins.h"

value_t builtin_null_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_NULL) {
        runtime_error(vm, "hash() can only be called on null");
    }
    
    // Null always hashes to 0
    return make_int32(0);
}

value_t builtin_null_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_NULL) {
        runtime_error(vm, "equals() can only be called on null");
    }
    
    // Only null equals null
    return make_boolean(other.type == VAL_NULL);
}

value_t builtin_null_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_NULL) {
        runtime_error(vm, "toString() can only be called on null");
    }
    
    return make_string("null");
}