#include "vm.h"

vm_result op_or(vm_t* vm) {
    // OR operator has JavaScript-like semantics:
    // Returns first operand if truthy, otherwise returns second operand
    // This should be implemented with short-circuit evaluation, but
    // since we have both operands on the stack already, we just
    // implement the value selection logic
    
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);
    
    // If a is truthy, return a; otherwise return b
    value_t result;
    if (!is_falsy(a)) {
        result = a;
        vm_retain(result);
        vm_release(b);
    } else {
        result = b;
        vm_retain(result);
        vm_release(a);
    }
    
    vm_push(vm, result);
    vm_release(result);
    
    return VM_OK;
}