#include "vm.h"
#include "module.h"
#include "runtime_error.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

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
    
    // For namespace imports, read the namespace name before loading the module
    // (loading the module may change the VM state)
    value_t namespace_name_value;
    if (is_namespace) {
        // Read namespace name constant index
        uint8_t namespace_idx = *vm->ip++;
        
        // Get the namespace name from constants (context-aware)
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
    }
    
    // For namespace imports, we'll try loading the module but handle failure differently
    // For other import types, we need the module to exist
    module_t* module = NULL;
    
    if (!is_namespace) {
        // For wildcard and specific imports, the module must exist
        module = module_load(vm, module_path);
        if (!module) {
            runtime_error(vm, "Module not found: %s", module_path);
            return VM_RUNTIME_ERROR;
        }
    } else {
        // For namespace imports, try to load but don't fail yet
        module = module_load(vm, module_path);
        // If it fails, we'll try the item import interpretation below
    }
    
    if (is_wildcard) {
        // Wildcard import - import all module exports into current globals
        // Skip the next byte (unused for wildcard)
        vm->ip++;
        
        
        // Copy all module exports to VM globals
        do_foreach_property(module->exports, copy_export_to_globals, vm);
        
    } else if (is_namespace) {
        // Namespace import - but need to handle ambiguous bare imports
        // For bare imports like "import a.b.c", we try:
        // 1. Load "a.b.c" as a module (namespace import)
        // 2. If that fails, load "a.b" and import item "c"
        
        // Check if we successfully loaded the module
        if (module) {
            // Success - treat as namespace import
            const char* namespace_name = namespace_name_value.as.string;
            
            // Create a new object to hold the module exports
            do_object namespace_obj = do_create(NULL);
            value_t namespace_object = make_object(namespace_obj);
            
            // Copy all module exports to the namespace object
            do_foreach_property(module->exports, copy_export_to_namespace_object, namespace_obj);
            
            // Add the namespace object to VM globals
            do_set(vm->globals, namespace_name, &namespace_object, sizeof(value_t));
        } else {
            // Module not found - try as item import by splitting at last dot
            const char* last_dot = strrchr(module_path, '.');
            if (last_dot && last_dot != module_path) {
                // Split into parent module and item name
                size_t parent_len = last_dot - module_path;
                char* parent_path = malloc(parent_len + 1);
                strncpy(parent_path, module_path, parent_len);
                parent_path[parent_len] = '\0';
                
                const char* item_name = last_dot + 1;
                
                // Try to load the parent module
                module_t* parent_module = module_load(vm, parent_path);
                
                if (parent_module) {
                    // Get the item from the parent module
                    value_t item_value = module_get_export(parent_module, item_name);
                    if (item_value.type != VAL_UNDEFINED) {
                        // Success - add the item to globals
                        do_set(vm->globals, item_name, &item_value, sizeof(value_t));
                        free(parent_path);
                    } else {
                        slate_runtime_error(vm, ERR_TYPE, __FILE__, __LINE__, -1, 
                            "Export '%s' not found in module %s", item_name, parent_path);
                        free(parent_path);
                        return VM_RUNTIME_ERROR;
                    }
                } else {
                    // Neither interpretation worked
                    free(parent_path);
                    runtime_error(vm, "Module not found: %s", module_path);
                    return VM_RUNTIME_ERROR;
                }
            } else {
                // No dot to split on, or dot is at the beginning
                runtime_error(vm, "Module not found: %s", module_path);
                return VM_RUNTIME_ERROR;
            }
        }
        
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