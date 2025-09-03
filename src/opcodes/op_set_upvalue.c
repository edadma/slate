#include "vm.h"
#include "runtime_error.h"

// OP_SET_UPVALUE: Set upvalue (operand = upvalue index)
// Stack before: [..., value]
// Stack after:  [...]
vm_result op_set_upvalue(vm_t* vm) {
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
    
    // Pop the value from stack and store in upvalue
    value_t value = vm_pop(vm);
    
    // Release old upvalue and retain new one
    vm_release(closure->upvalues[upvalue_index]);
    closure->upvalues[upvalue_index] = vm_retain(value);
    
    // Release the popped value
    vm_release(value);
    
    return VM_OK;
}