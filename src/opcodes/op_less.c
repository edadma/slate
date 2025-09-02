#include "vm.h"

vm_result op_less(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Handle all numeric type combinations for comparison
    if (is_number(a) && is_number(b)) {
        vm_push(vm, make_boolean_with_debug(compare_numbers(a, b) < 0, a.debug));
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