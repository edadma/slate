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