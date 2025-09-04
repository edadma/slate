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

// Number method: equals(other) - Cross-type numeric equality comparison
value_t builtin_number_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    // Check that receiver is numeric
    if (!is_number(receiver)) {
        runtime_error(vm, "equals() can only be called on numbers");
    }
    
    // Non-numbers are never equal to numbers
    if (!is_number(other)) {
        return make_boolean(false);
    }
    
    // Handle same-type comparisons for performance
    if (receiver.type == other.type) {
        switch (receiver.type) {
        case VAL_INT32:
            return make_boolean(receiver.as.int32 == other.as.int32);
        case VAL_BIGINT:
            return make_boolean(di_eq(receiver.as.bigint, other.as.bigint));
        case VAL_FLOAT32: {
            // IEEE 754: NaN is never equal to NaN
            float fa = receiver.as.float32, fb = other.as.float32;
            if (isnan(fa) || isnan(fb)) return make_boolean(false);
            return make_boolean(fa == fb);
        }
        case VAL_FLOAT64: {
            // IEEE 754: NaN is never equal to NaN
            double da = receiver.as.float64, db = other.as.float64;
            if (isnan(da) || isnan(db)) return make_boolean(false);
            return make_boolean(da == db);
        }
        default:
            return make_boolean(false);
        }
    }
    
    // Cross-type numeric comparisons - convert to common type (double)
    double a_val = (receiver.type == VAL_INT32) ? (double)receiver.as.int32
        : (receiver.type == VAL_BIGINT)         ? di_to_double(receiver.as.bigint)
        : (receiver.type == VAL_FLOAT32)        ? (double)receiver.as.float32
                                                : receiver.as.float64;
    double b_val = (other.type == VAL_INT32) ? (double)other.as.int32
        : (other.type == VAL_BIGINT)         ? di_to_double(other.as.bigint)
        : (other.type == VAL_FLOAT32)        ? (double)other.as.float32
                                             : other.as.float64;
    
    // IEEE 754: NaN is never equal to NaN, Infinity equals Infinity across precisions
    if (isnan(a_val) || isnan(b_val)) return make_boolean(false);
    return make_boolean(a_val == b_val);
}