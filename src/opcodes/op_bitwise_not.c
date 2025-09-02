#include "vm.h"

vm_result op_bitwise_not(vm_t* vm) {
    value_t a = vm_pop(vm);

    // Operand must be an integer for bitwise operations
    if (!is_number(a)) {
        vm_runtime_error_with_values(vm, "Cannot perform bitwise NOT on %s", &a, NULL, a.debug);
        vm_release(a);
        return VM_RUNTIME_ERROR;
    }

    // Convert to integer for bitwise operation
    int32_t a_int = value_to_int(a);
    
    vm_push(vm, make_int32_with_debug(~a_int, a.debug));

    vm_release(a);
    return VM_OK;
}