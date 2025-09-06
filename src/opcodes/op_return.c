#include "vm.h"
#include "module.h"

vm_result op_return(vm_t* vm) {
    // Get return value from top of stack
    value_t result = vm_pop(vm);
    
    // Get frame we're returning from BEFORE decrementing frame_count
    call_frame* current_frame = &vm->frames[vm->frame_count - 1];  // Frame we're in
    
    // Pop module context if this function had one
    if (current_frame->closure && current_frame->closure->module) {
        module_pop_context(vm);
    }
    
    // Restore previous call frame
    vm->frame_count--;
    if (vm->frame_count == 0) {
        // Returning from main - set result and halt
        vm->result = result;
        return VM_OK;
    }
    
    // Get previous frame (now the active frame)
    call_frame* prev_frame = &vm->frames[vm->frame_count - 1];  // Frame to return to
    
    // Clean up stack (remove local variables and arguments)
    vm->stack_top = current_frame->slots;
    
    // Push return value
    vm_push(vm, result);
    
    // Restore execution context - use the return address saved in the current frame
    vm->ip = current_frame->ip;  // This has the return address saved during CALL
    vm->bytecode = prev_frame->closure->function->bytecode;
    
    return VM_OK;
}