#include "vm.h"
#include <math.h>

vm_result op_power(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations for power operations
    if (is_number(a) && is_number(b)) {

        // Convert to double for power calculation (always returns float)
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                             : a.as.number;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                             : b.as.number;

        vm_push(vm, make_number_with_debug(pow(a_val, b_val), a.debug));
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

        vm_runtime_error_with_values(vm, "Cannot compute power of %s and %s", &a, &b, error_debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    vm_release(b);
    return VM_OK;
}