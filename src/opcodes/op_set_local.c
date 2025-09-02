#include "vm.h"

vm_result op_set_local(vm_t* vm) {
    uint8_t slot = *vm->ip++;
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    
    // Release old value first (proper memory management)
    vm_release(frame->slots[slot]);
    
    // Set new value (peek doesn't pop - assignment is expression)
    frame->slots[slot] = vm_retain(vm_peek(vm, 0));
    return VM_OK;
}