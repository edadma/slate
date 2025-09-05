#include "vm.h"
#include "runtime_error.h"
#include "module.h"

// Helper function to get the appropriate namespace for global operations
static inline do_object get_current_namespace(vm_t* vm) {
    module_t* current_module = module_get_current_context(vm);
    return current_module ? current_module->namespace : vm->globals;
}

vm_result op_define_global(vm_t* vm) {
    // Pop the value to store and the variable name constant
    value_t value = vm_pop(vm);

    // Note: We allow undefined for variable declarations (var x;)
    // The restriction on undefined only applies to explicit assignments

    uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Read immutability flag (1 byte)
    uint8_t is_immutable = *vm->ip;
    vm->ip++;

    // Get the current executing function from the current frame
    function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
    if (name_constant >= current_func->constant_count) {
        vm_release(value);
        runtime_error(vm, "Constant index out of bounds in OP_DEFINE_GLOBAL");
        return VM_RUNTIME_ERROR;
    }
    
    value_t name_val = current_func->constants[name_constant];
    if (name_val.type != VAL_STRING) {
        vm_release(value);
        runtime_error(vm, "Global variable name must be a string");
        return VM_RUNTIME_ERROR;
    }

    // Get the appropriate namespace (module namespace or VM globals)
    do_object target_namespace = get_current_namespace(vm);
    
    // Check if variable already exists (prevent redeclaration in scripts, allow in REPL)
    value_t* existing_value = (value_t*)do_get(target_namespace, name_val.as.string);
    if (existing_value && vm->context == CTX_SCRIPT) {
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), "Variable '%s' is already declared", name_val.as.string);
        vm_release(value);
        runtime_error(vm, "%s", error_msg);
        return VM_RUNTIME_ERROR;
    }

    // Handle variable storage (new declaration or REPL redeclaration)
    value_t* stored_value;
    bool* immutable_flag;
    
    if (existing_value) {
        // REPL mode redeclaration - reuse existing storage
        vm_release(*existing_value);  // Release old value
        *existing_value = value;      // Store new value
        stored_value = existing_value;
        
        // Update immutability flag
        immutable_flag = (bool*)do_get(vm->global_immutability, name_val.as.string);
        if (immutable_flag) {
            *immutable_flag = (bool)is_immutable;
        } else {
            // This shouldn't happen, but handle gracefully
            immutable_flag = malloc(sizeof(bool));
            if (!immutable_flag) {
                runtime_error(vm, "Memory allocation failed for immutability flag");
                return VM_RUNTIME_ERROR;
            }
            *immutable_flag = (bool)is_immutable;
            do_set(vm->global_immutability, name_val.as.string, immutable_flag, sizeof(bool));
        }
    } else {
        // New declaration - allocate new storage
        stored_value = malloc(sizeof(value_t));
        if (!stored_value) {
            vm_release(value);
            runtime_error(vm, "Memory allocation failed");
            return VM_RUNTIME_ERROR;
        }
        *stored_value = value;
        
        // ds_string can be used directly as char* - no ds_cstr needed
        // do_set needs key, data pointer, and size
        do_set(target_namespace, name_val.as.string, stored_value, sizeof(value_t));
        
        // Store immutability flag in parallel object
        immutable_flag = malloc(sizeof(bool));
        if (!immutable_flag) {
            vm_release(value);
            runtime_error(vm, "Memory allocation failed for immutability flag");
            return VM_RUNTIME_ERROR;
        }
        *immutable_flag = (bool)is_immutable;
        do_set(vm->global_immutability, name_val.as.string, immutable_flag, sizeof(bool));
    }
    return VM_OK;
}