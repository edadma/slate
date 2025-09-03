#include "vm.h"
#include "runtime_error.h"

// OP_GET_UPVALUE: Get upvalue (operand = upvalue index)
// Stack before: [...]
// Stack after:  [..., upvalue]
vm_result op_get_upvalue(vm_t* vm) {
    // Get the upvalue index from the next byte
    uint8_t upvalue_index = *vm->ip++;
    
    // Get the current call frame
    if (vm->frame_count == 0) {
        runtime_error(vm, "Cannot access upvalue outside of function");
        return VM_RUNTIME_ERROR;
    }
    
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    closure_t* closure = frame->closure;
    
    // Check bounds
    if (upvalue_index >= closure->upvalue_count) {
        runtime_error(vm, "Upvalue index out of bounds");
        return VM_RUNTIME_ERROR;
    }
    
    // Get the upvalue and push to stack
    value_t upvalue = closure->upvalues[upvalue_index];
    vm_push(vm, upvalue);
    
    return VM_OK;
}