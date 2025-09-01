#include "vm.h"
#include <math.h>

vm_result op_floor_div(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Both operands must be numbers
    if (!is_number(a) || !is_number(b)) {
        vm_runtime_error_with_values(vm, "Cannot perform floor division on %s and %s", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Check for division by zero
    if ((b.type == VAL_INT32 && b.as.int32 == 0) ||
        (b.type == VAL_NUMBER && b.as.number == 0.0) ||
        (b.type == VAL_BIGINT && di_is_zero(b.as.bigint))) {
        vm_runtime_error_with_debug(vm, "Division by zero");
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Convert both to double for floor division
    double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
        : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                         : a.as.number;
    double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
        : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                         : b.as.number;

    // Perform floor division
    double result = floor(a_val / b_val);
    
    // If result fits in int32, return as int32, otherwise as double
    if (result >= INT32_MIN && result <= INT32_MAX && result == floor(result)) {
        vm_push(vm, make_int32_with_debug((int32_t)result, a.debug));
    } else {
        vm_push(vm, make_number_with_debug(result, a.debug));
    }

    vm_release(a);
    vm_release(b);
    return VM_OK;
}