#include "vm.h"

vm_result op_subtract(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations
    if (is_number(a) && is_number(b)) {

        // int32 - int32 with overflow detection
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            int32_t result;
            if (di_subtract_overflow_int32(a.as.int32, b.as.int32, &result)) {
                vm_push(vm, make_int32_with_debug(result, a.debug));
            } else {
                // Overflow - promote to BigInt
                int64_t big_result = (int64_t)a.as.int32 - (int64_t)b.as.int32;
                di_int big = di_from_int64(big_result);
                vm_push(vm, make_bigint_with_debug(big, a.debug));
            }
        }
        // Mixed with floating point - convert to double
        else {
            double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
                : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                                 : a.as.number;
            double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
                : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                                 : b.as.number;
            vm_push(vm, make_number_with_debug(a_val - b_val, a.debug));
        }
    } else {
        // For subtraction, determine which operand is problematic
        debug_location* error_debug = NULL;
        if (!is_number(a) && is_number(b)) {
            error_debug = a.debug; // Left operand is problematic
        } else if (is_number(a) && !is_number(b)) {
            error_debug = b.debug; // Right operand is problematic
        } else {
            error_debug = a.debug; // Both problematic, use left
        }

        vm_runtime_error_with_values(vm, "Cannot subtract %s and %s", &a, &b, error_debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    vm_release(b);
    return VM_OK;
}