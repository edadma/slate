#include "vm.h"

vm_result op_null_coalesce(slate_vm* vm) {
    value_t b = vm_pop(vm);  // Right operand (fallback value)
    value_t a = vm_pop(vm);  // Left operand (primary value)

    // If 'a' is null or undefined, use 'b', otherwise use 'a'
    if (a.type == VAL_NULL || a.type == VAL_UNDEFINED) {
        vm_push(vm, vm_retain(b));
        vm_release(a);
        vm_release(b);
    } else {
        vm_push(vm, vm_retain(a));
        vm_release(a);
        vm_release(b);
    }
    
    return VM_OK;
}