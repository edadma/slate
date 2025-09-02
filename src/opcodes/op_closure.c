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
    
    // For now, create a simple closure (no upvalues)
    closure_t* new_closure = closure_create(target_func);
    if (!new_closure) {
        runtime_error(vm, "Failed to create closure");
        return VM_RUNTIME_ERROR;
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