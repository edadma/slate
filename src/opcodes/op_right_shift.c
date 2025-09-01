#include "vm.h"

vm_result op_right_shift(slate_vm* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);

    // Both operands must be integers for shift operations
    if (!is_number(a) || !is_number(b)) {
        vm_runtime_error_with_values(vm, "Cannot perform right shift on %s and %s", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }

    // Convert to integers for shift operation
    int32_t a_int = value_to_int(a);
    int32_t b_int = value_to_int(b);
    
    // Handle negative shift amounts as error
    if (b_int < 0) {
        vm_runtime_error_with_debug(vm, "Shift amount cannot be negative");
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    
    // For shift amounts >= 32, take modulo 32 (JavaScript-style behavior)
    b_int = b_int % 32;
    
    // Arithmetic right shift (sign extending)
    vm_push(vm, make_int32_with_debug(a_int >> b_int, a.debug));

    vm_release(a);
    vm_release(b);
    return VM_OK;
}