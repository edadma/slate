#include "vm.h"
#include "module.h"
#include "runtime_error.h"
#include <stdio.h>

// Callback to copy an export to VM globals for wildcard import
void copy_export_to_globals(const char* key, void* data, size_t size, void* context) {
    vm_t* vm = (vm_t*)context;
    if (!vm || !key || !data || size != sizeof(value_t)) {
        return;
    }
    
    value_t* value = (value_t*)data;
    do_set(vm->globals, key, value, sizeof(value_t));
}

// Callback to copy an export to namespace object for namespace import
void copy_export_to_namespace_object(const char* key, void* data, size_t size, void* context) {
    do_object namespace_obj = (do_object)context;
    if (!namespace_obj || !key || !data || size != sizeof(value_t)) {
        return;
    }
    
    value_t* value = (value_t*)data;
    do_set(namespace_obj, key, value, sizeof(value_t));
}

// Import module operation
// Bytecode format: OP_IMPORT_MODULE module_path_constant_index flags specifier_count [specifiers...]
vm_result op_import_module(vm_t* vm) {
    if (!vm) {
        return VM_RUNTIME_ERROR;
    }
    
    // Read module path constant index (16-bit operand)
    uint16_t path_constant_index = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Get module path from constants (context-aware)
    value_t path_value;
    if (vm->frame_count == 0) {
        // Main program execution
        if (path_constant_index >= vm->constant_count) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
            return VM_RUNTIME_ERROR;
        }
        path_value = vm->constants[path_constant_index];
    } else {
        // Function execution
        function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
        if (path_constant_index >= current_func->constant_count) {
            slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
            return VM_RUNTIME_ERROR;
        }
        path_value = current_func->constants[path_constant_index];
    }
    
    if (path_value.type != VAL_STRING) {
        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Module path must be a string");
        return VM_RUNTIME_ERROR;
    }
    
    const char* module_path = path_value.as.string;
    
    // Read flags byte (0xFF = wildcard, 0xFE = namespace, otherwise specifier count)
    uint8_t flags = *vm->ip++;
    int is_wildcard = (flags == 0xFF);
    int is_namespace = (flags == 0xFE);
    
    // Load the module
    module_t* module = module_load(vm, module_path);
    if (!module) {
        printf("Runtime error: Module not found: %s\n", module_path);
        return VM_RUNTIME_ERROR;
    }
    
    if (is_wildcard) {
        // Wildcard import - import all module exports into current globals
        // Skip the next byte (unused for wildcard)
        vm->ip++;
        
        
        // Copy all module exports to VM globals
        do_foreach_property(module->exports, copy_export_to_globals, vm);
        
    } else if (is_namespace) {
        // Namespace import - create namespace object containing all module exports
        
        // Read namespace name constant index
        uint8_t namespace_idx = *vm->ip++;
        
        // Get the namespace name from constants (context-aware)
        value_t namespace_name_value;
        if (vm->frame_count == 0) {
            // Main program execution
            if (namespace_idx >= vm->constant_count) {
                slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
                return VM_RUNTIME_ERROR;
            }
            namespace_name_value = vm->constants[namespace_idx];
        } else {
            // Function execution
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            if (namespace_idx >= current_func->constant_count) {
                slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
                return VM_RUNTIME_ERROR;
            }
            namespace_name_value = current_func->constants[namespace_idx];
        }
        
        if (namespace_name_value.type != VAL_STRING) {
            slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Namespace name must be a string");
            return VM_RUNTIME_ERROR;
        }
        
        const char* namespace_name = namespace_name_value.as.string;
        
        // Create a new object to hold the module exports
        do_object namespace_obj = do_create(NULL);
        value_t namespace_object = make_object(namespace_obj);
        
        // Copy all module exports to the namespace object
        do_foreach_property(module->exports, copy_export_to_namespace_object, namespace_obj);
        
        // Add the namespace object to VM globals
        do_set(vm->globals, namespace_name, &namespace_object, sizeof(value_t));
        
    } else {
        // Specific imports
        uint8_t specifier_count = flags;
        
        
        // Read each specifier (name_constant_index, alias_constant_index)
        for (int i = 0; i < specifier_count; i++) {
            uint8_t name_idx = *vm->ip++;
            uint8_t alias_idx = *vm->ip++;
            
            // Get the name and alias from constants (context-aware)
            value_t name_value, alias_value;
            
            if (vm->frame_count == 0) {
                // Main program execution
                if (name_idx >= vm->constant_count || alias_idx >= vm->constant_count) {
                    slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
                    return VM_RUNTIME_ERROR;
                }
                name_value = vm->constants[name_idx];
                alias_value = vm->constants[alias_idx];
            } else {
                // Function execution
                function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
                if (name_idx >= current_func->constant_count || alias_idx >= current_func->constant_count) {
                    slate_runtime_error(vm, ERR_ASSERT, __FILE__, __LINE__, -1, "Constant index out of bounds");
                    return VM_RUNTIME_ERROR;
                }
                name_value = current_func->constants[name_idx];
                alias_value = current_func->constants[alias_idx];
            }
            
            if (name_value.type != VAL_STRING || alias_value.type != VAL_STRING) {
                slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Import specifier names must be strings");
                return VM_RUNTIME_ERROR;
            }
            
            const char* export_name = name_value.as.string;
            const char* local_name = alias_value.as.string;
            
            // Get the exported value
            value_t exported_value = module_get_export(module, export_name);
            if (exported_value.type == VAL_UNDEFINED) {
                slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, "Export '%s' not found in module %s", export_name, module_path);
                return VM_RUNTIME_ERROR;
            }
            
            // Add to current globals with the local name
            do_set(vm->globals, local_name, &exported_value, sizeof(value_t));
            
        }
    }
    
    return VM_OK;
}