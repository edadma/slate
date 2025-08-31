#include "vm.h"

vm_result op_negate(slate_vm* vm) {
    value_t a = vm_pop(vm);
    if (a.type == VAL_INT32) {
        // Check for int32 overflow on negation
        if (a.as.int32 == INT32_MIN) {
            // INT32_MIN negation overflows - promote to BigInt
            di_int big = di_from_int64(-((int64_t)INT32_MIN));
            vm_push(vm, make_bigint_with_debug(big, a.debug));
        } else {
            vm_push(vm, make_int32_with_debug(-a.as.int32, a.debug));
        }
    } else if (a.type == VAL_BIGINT) {
        di_int negated = di_negate(a.as.bigint);
        vm_push(vm, make_bigint_with_debug(negated, a.debug));
    } else if (a.type == VAL_NUMBER) {
        vm_push(vm, make_number_with_debug(-a.as.number, a.debug));
    } else {
        vm_runtime_error_with_values(vm, "Cannot negate %s", &a, NULL, a.debug);
        vm_release(a);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    return VM_OK;
}