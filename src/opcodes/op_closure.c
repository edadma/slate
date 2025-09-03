#include "vm.h"
#include "runtime_error.h"

vm_result op_closure(vm_t* vm) {
    uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Get function index from constants
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    value_t index_val = current_func->constants[constant];
    if (index_val.type != VAL_INT32) {
        runtime_error(vm, "Expected function index in OP_CLOSURE");
        return VM_RUNTIME_ERROR;
    }
    
    // Get function from function table
    size_t func_index = (size_t)index_val.as.int32;
    function_t* target_func = vm_get_function(vm, func_index);
    if (!target_func) {
        runtime_error(vm, "Invalid function index in OP_CLOSURE");
        return VM_RUNTIME_ERROR;
    }
    
    // Create closure with upvalues
    closure_t* new_closure = closure_create(target_func);
    if (!new_closure) {
        runtime_error(vm, "Failed to create closure");
        return VM_RUNTIME_ERROR;
    }
    
    // Populate upvalues based on function descriptors
    if (target_func->upvalue_count > 0) {
        new_closure->upvalue_count = target_func->upvalue_count;
        new_closure->upvalues = malloc(sizeof(value_t) * target_func->upvalue_count);
        if (!new_closure->upvalues) {
            closure_destroy(new_closure);
            runtime_error(vm, "Failed to allocate upvalue array");
            return VM_RUNTIME_ERROR;
        }
        
        // Current closure context for capturing variables
        call_frame* current_frame = &vm->frames[vm->frame_count - 1];
        
        // Capture each upvalue
        for (size_t i = 0; i < target_func->upvalue_count; i++) {
            upvalue_desc_t* desc = &target_func->upvalue_descriptors[i];
            
            if (desc->is_local) {
                // Capture from current frame's local variables
                value_t captured_value = current_frame->slots[desc->index];
                new_closure->upvalues[i] = vm_retain(captured_value);
            } else {
                // Capture from current closure's upvalues
                value_t captured_value = current_frame->closure->upvalues[desc->index];
                new_closure->upvalues[i] = vm_retain(captured_value);
            }
        }
    }
    
    // Push closure as a value onto the stack
    value_t closure_val;
    closure_val.type = VAL_CLOSURE;
    closure_val.as.closure = new_closure;
    closure_val.class = NULL;
    closure_val.debug = NULL;
    vm_push(vm, closure_val);
    return VM_OK;
}