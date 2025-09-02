#include "vm.h"

vm_result op_bitwise_xor(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Both operands must be integers for bitwise operations
    if (!is_number(a) || !is_number(b)) {
        vm_runtime_error_with_values(vm, "Cannot perform bitwise XOR on %s and %s", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Convert to integers for bitwise operation
    int32_t a_int = value_to_int(a);
    int32_t b_int = value_to_int(b);
    
    vm_push(vm, make_int32_with_debug(a_int ^ b_int, a.debug));

    vm_release(a);
    vm_release(b);
    return VM_OK;
}