#include "class_boolean.h"
#include "builtins.h"
#include "dynamic_string.h"

// Boolean method: hash
value_t builtin_boolean_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "hash() can only be called on booleans");
    }

    // Hash booleans as 0 for false, 1 for true
    return make_int32(receiver.as.boolean ? 1 : 0);
}

// Boolean method: toString
value_t builtin_boolean_to_string(vm_t* vm, int arg_count, value_t* args) {
    // When called as a method, args[0] is the receiver (the boolean)
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "toString() can only be called on booleans");
    }

    // Return "true" or "false" as a string
    if (receiver.as.boolean) {
        return make_string("true");
    } else {
        return make_string("false");
    }
}

// Boolean method: and(other) - logical AND with another value
value_t builtin_boolean_and(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "and() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "and() can only be called on booleans");
    }

    // Boolean AND: if receiver is false, return false; otherwise return truthiness of other
    if (!receiver.as.boolean) {
        return make_boolean(false);
    } else {
        return make_boolean(is_truthy(other));
    }
}

// Boolean method: or(other) - logical OR with another value
value_t builtin_boolean_or(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "or() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "or() can only be called on booleans");
    }

    // Boolean OR: if receiver is true, return true; otherwise return truthiness of other
    if (receiver.as.boolean) {
        return make_boolean(true);
    } else {
        return make_boolean(is_truthy(other));
    }
}

// Boolean method: not() - logical NOT
value_t builtin_boolean_not(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "not() takes no arguments (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "not() can only be called on booleans");
    }

    // Return the logical negation
    return make_boolean(!receiver.as.boolean);
}

// Boolean method: xor(other) - logical XOR with another value
value_t builtin_boolean_xor(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "xor() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_BOOLEAN) {
        runtime_error(vm, "xor() can only be called on booleans");
    }

    // Boolean XOR: return true if exactly one is truthy
    bool receiver_bool = receiver.as.boolean;
    bool other_bool = is_truthy(other);
    return make_boolean(receiver_bool != other_bool);
}