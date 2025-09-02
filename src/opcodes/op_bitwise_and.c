#include "vm.h"

vm_result op_bitwise_and(vm_t* vm) {
    value_t b = vm_pop(vm);
    value_t a = vm_pop(vm);
    
    if (a.type == VAL_INT32 && b.type == VAL_INT32) {
        vm_push(vm, make_int32_with_debug(a.as.int32 & b.as.int32, a.debug));
    } else {
        vm_runtime_error_with_values(vm, "Bitwise AND requires integers", &a, &b, a.debug);
        vm_release(a);
        vm_release(b);
        return VM_RUNTIME_ERROR;
    }
    
    vm_release(a);
    vm_release(b);
    return VM_OK;
}