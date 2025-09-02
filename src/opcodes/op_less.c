#include "vm.h"

vm_result op_less(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations for comparison
    if (is_number(a) && is_number(b)) {

        // Convert both to double for comparison (simple but works)
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                             : a.as.float64;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                             : b.as.float64;

        vm_push(vm, make_boolean_with_debug(a_val < b_val, a.debug));
    } else {
        vm_runtime_error_with_values(vm, "Can only compare numbers", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    vm_release(a);
    vm_release(b);
    return VM_OK;
}