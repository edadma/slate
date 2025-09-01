#include "vm.h"
#include "runtime_error.h"

vm_result op_divide(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations
    if (is_number(a) && is_number(b)) {

        // Check for division by zero
        bool is_zero = false;
        if (b.type == VAL_INT32 && b.as.int32 == 0)
            is_zero = true;
        else if (b.type == VAL_BIGINT && di_is_zero(b.as.bigint))
            is_zero = true;
        else if (b.type == VAL_NUMBER && b.as.number == 0)
            is_zero = true;

        if (is_zero) {
            // Release values before throwing error
            vm_release(a);
            vm_release(b);
            
            // Throw the error with automatic debug location extraction - this never returns
            slate_runtime_error_with_debug(vm, ERR_ARITHMETIC, &a, &b, "Division by zero");
            // No return needed - runtime_error never returns
        }

        // Division always produces floating point result for simplicity
        // (matches Python 3 behavior: 5 / 2 = 2.5)
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                             : a.as.number;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                             : b.as.number;
        vm_push(vm, make_number_with_debug(a_val / b_val, a.debug));
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

        vm_runtime_error_with_values(vm, "Cannot divide %s and %s", &a, &b, error_debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    vm_release(b);
    return VM_OK;
}