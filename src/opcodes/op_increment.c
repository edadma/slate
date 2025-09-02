#include "vm.h"

vm_result op_increment(vm_t* vm) {
    value_t a = vm_pop(vm);

    // Operand must be a number
    if (!is_number(a)) {
        vm_runtime_error_with_values(vm, "Cannot increment %s", &a, NULL, a.debug);
        vm_release(a);
        return VM_RUNTIME_ERROR;
    }

    // Handle different number types
    if (a.type == VAL_INT32) {
        // Check for overflow
        if (a.as.int32 == INT32_MAX) {
            // Promote to BigInt on overflow
            di_int result = di_from_int64((int64_t)a.as.int32 + 1);
            vm_push(vm, make_bigint_with_debug(result, a.debug));
        } else {
            vm_push(vm, make_int32_with_debug(a.as.int32 + 1, a.debug));
        }
    } else if (a.type == VAL_BIGINT) {
        di_int result = di_add_i32(a.as.bigint, 1);
        vm_push(vm, make_bigint_with_debug(result, a.debug));
    } else { // VAL_FLOAT64
        vm_push(vm, make_float64_with_debug(a.as.float64 + 1.0, a.debug));
    }

    vm_release(a);
    return VM_OK;
}