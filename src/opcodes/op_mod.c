#include "vm.h"
#include <math.h>

vm_result op_mod(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations
    if (is_number(a) && is_number(b)) {

        // Check for modulo by zero
        bool is_zero = false;
        if (b.type == VAL_INT32 && b.as.int32 == 0)
            is_zero = true;
        else if (b.type == VAL_BIGINT && di_is_zero(b.as.bigint))
            is_zero = true;
        else if (b.type == VAL_FLOAT64 && b.as.float64 == 0)
            is_zero = true;

        if (is_zero) {
            vm_runtime_error_with_values(vm, "Modulo by zero", &a, &b, b.debug);
            vm_release(a);
            vm_release(b);
            return VM_RUNTIME_ERROR;
        }

        // int32 % int32
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            // No overflow possible with modulo
            vm_push(vm, make_int32_with_debug(a.as.int32 % b.as.int32, a.debug));
        }
        // BigInt % BigInt
        else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            di_int result = di_mod(a.as.bigint, b.as.bigint);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // int32 % BigInt
        else if (a.type == VAL_INT32 && b.type == VAL_BIGINT) {
            di_int a_big = di_from_int32(a.as.int32);
            di_int result = di_mod(a_big, b.as.bigint);
            di_release(&a_big);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // BigInt % int32
        else if (a.type == VAL_BIGINT && b.type == VAL_INT32) {
            di_int b_big = di_from_int32(b.as.int32);
            di_int result = di_mod(a.as.bigint, b_big);
            di_release(&b_big);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        }
        // Mixed with floating point - handle float32/float64 promotion
        else {
            // Determine result type based on operands (promote to highest precision)
            int has_float64 = (a.type == VAL_FLOAT64) || (b.type == VAL_FLOAT64);
            
            if (has_float64) {
                // Promote to float64
                double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
                    : (a.type == VAL_BIGINT) ? di_to_double(a.as.bigint)
                    : (a.type == VAL_FLOAT32) ? (double)a.as.float32
                    : a.as.float64;
                double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
                    : (b.type == VAL_BIGINT) ? di_to_double(b.as.bigint)
                    : (b.type == VAL_FLOAT32) ? (double)b.as.float32
                    : b.as.float64;
                vm_push(vm, make_float64_with_debug(fmod(a_val, b_val), a.debug));
            } else {
                // Both are float32 or promote to float32
                float a_val = (a.type == VAL_INT32) ? (float)a.as.int32
                    : (a.type == VAL_BIGINT) ? (float)di_to_double(a.as.bigint)
                    : a.as.float32;
                float b_val = (b.type == VAL_INT32) ? (float)b.as.int32
                    : (b.type == VAL_BIGINT) ? (float)di_to_double(b.as.bigint)
                    : b.as.float32;
                vm_push(vm, make_float32_with_debug(fmodf(a_val, b_val), a.debug));
            }
        }
    } else {
        // Find the first non-numeric operand for error location
        debug_location* error_debug = NULL;
        if (!is_number(a) && is_number(b)) {
            error_debug = a.debug; // Left operand is problematic
        } else if (is_number(a) && !is_number(b)) {
            error_debug = b.debug; // Right operand is problematic
        } else {
            error_debug = a.debug; // Both problematic, use left
        }

        vm_runtime_error_with_values(vm, "Cannot compute modulo of %s and %s", &a, &b, error_debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    vm_release(b);
    return VM_OK;
}