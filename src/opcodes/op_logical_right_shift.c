#include "vm.h"

vm_result op_logical_right_shift(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Both operands must be integers for shift operations
    if (!is_number(a) || !is_number(b)) {
        vm_runtime_error_with_values(vm, "Cannot perform logical right shift on %s and %s", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Convert to integers for shift operation
    int32_t a_int = value_to_int(a);
    int32_t b_int = value_to_int(b);
    
    // Ensure shift amount is reasonable (0-31 for 32-bit integers)
    if (b_int < 0 || b_int >= 32) {
        vm_runtime_error_with_debug(vm, "Shift amount out of range");
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    
    // Logical right shift (zero-filling) - cast to unsigned for proper behavior
    uint32_t a_uint = (uint32_t)a_int;
    uint32_t result = a_uint >> b_int;
    
    vm_push(vm, make_int32_with_debug((int32_t)result, a.debug));

    vm_release(a);
    vm_release(b);
    return VM_OK;
}