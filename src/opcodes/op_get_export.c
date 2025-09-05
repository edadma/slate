#include "vm.h"
#include "module.h"
#include "runtime_error.h"

// Get export from a module
// Stack: [..., module, export_name] -> [..., exported_value]
vm_result op_get_export(vm_t* vm) {
    if (!vm) {
        return VM_RUNTIME_ERROR;
    }
    
    // Pop export name from stack
    value_t export_name = vm_pop(vm);
    if (export_name.type != VAL_STRING) {
        runtime_error(vm, "Export name must be a string");
        return VM_RUNTIME_ERROR;
    }
    
    // Pop module from stack
    value_t module_value = vm_pop(vm);
    if (module_value.type != VAL_OBJECT) {
        runtime_error(vm, "Expected module object");
        return VM_RUNTIME_ERROR;
    }
    
    // For now, modules are represented as objects containing their exports
    do_object module_obj = module_value.as.object;
    
    // Get the exported value
    if (do_has(module_obj, export_name.as.string)) {
        value_t* exported = (value_t*)do_get(module_obj, export_name.as.string);
        if (exported) {
            vm_push(vm, *exported);
            return VM_OK;
        }
    }
    
    // Export not found
    runtime_error(vm, "Export '%s' not found in module", export_name.as.string);
    return VM_RUNTIME_ERROR;
}