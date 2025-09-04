#include "vm.h"
#include "runtime_error.h"
#include "ast.h"

// ADT constructor factory function
static value_t adt_constructor_factory(vm_t* vm, int arg_count, value_t* args, 
                                       const char* case_name, data_case_type case_type, 
                                       char** param_names, size_t param_count) {
    if (case_type == DATA_CASE_SINGLETON) {
        // Singleton case: should not be called with arguments
        if (arg_count > 0) {
            runtime_error(vm, "Singleton case '%s' cannot be called with arguments", case_name);
            return make_null();
        }
        
        // Return a singleton instance
        value_t instance = make_object_with_debug(NULL, vm->current_debug);
        
        // Set type information on the instance
        do_set(instance.as.object, "__type", ds_new(case_name), sizeof(ds_string));
        do_set(instance.as.object, "__case_type", ds_new("singleton"), sizeof(ds_string));
        
        return instance;
    } else {
        // Constructor case: validate argument count
        if (arg_count != (int)param_count) {
            runtime_error(vm, "Constructor '%s' expects %zu arguments but got %d", 
                         case_name, param_count, arg_count);
            return make_null();
        }
        
        // Create instance with argument values
        value_t instance = make_object_with_debug(NULL, vm->current_debug);
        
        // Set type information
        do_set(instance.as.object, "__type", ds_new(case_name), sizeof(ds_string));
        do_set(instance.as.object, "__case_type", ds_new("constructor"), sizeof(ds_string));
        
        // Set parameter values
        for (size_t i = 0; i < param_count; i++) {
            value_t* arg_copy = malloc(sizeof(value_t));
            if (arg_copy) {
                *arg_copy = vm_retain(args[i]);
                do_set(instance.as.object, param_names[i], arg_copy, sizeof(value_t));
            }
        }
        
        return instance;
    }
}

// Wrapper factory function that creates ADT instances
static value_t adt_constructor_wrapper(vm_t* vm, int arg_count, value_t* args) {
    // Suppress unused parameter warnings for simplified implementation
    (void)arg_count;
    (void)args;
    
    // Create a new object instance
    do_object object = do_create(NULL);
    if (!object) {
        return make_null();
    }
    
    value_t instance = make_object_with_debug(object, vm->current_debug);
    
    return instance;
}

vm_result op_create_adt_constructor(vm_t* vm) {
    // Read parameter count from operand
    uint16_t param_count = *vm->ip | (*(vm->ip + 1) << 8);
    vm->ip += 2;
    
    // Stack contains: [name, case_type, param_count, param_name1, param_name2, ...]
    
    // Pop parameter names
    char** param_names = NULL;
    if (param_count > 0) {
        param_names = malloc(sizeof(char*) * param_count);
        if (!param_names) {
            runtime_error(vm, "Memory allocation failed for parameter names");
            return VM_RUNTIME_ERROR;
        }
        
        // Pop parameter names in reverse order
        for (int i = param_count - 1; i >= 0; i--) {
            value_t param_name_val = vm_pop(vm);
            if (param_name_val.type != VAL_STRING) {
                // Clean up and error
                for (int j = i + 1; j < (int)param_count; j++) {
                    free(param_names[j]);
                }
                free(param_names);
                vm_release(param_name_val);
                runtime_error(vm, "Parameter name must be a string");
                return VM_RUNTIME_ERROR;
            }
            param_names[i] = strdup(param_name_val.as.string);
            vm_release(param_name_val);
        }
    }
    
    // Pop remaining values
    value_t param_count_val = vm_pop(vm);
    value_t case_type_val = vm_pop(vm);
    value_t name_val = vm_pop(vm);
    
    if (name_val.type != VAL_STRING || case_type_val.type != VAL_INT32 || param_count_val.type != VAL_INT32) {
        // Clean up
        if (param_names) {
            for (size_t i = 0; i < param_count; i++) {
                free(param_names[i]);
            }
            free(param_names);
        }
        vm_release(param_count_val);
        vm_release(case_type_val);
        vm_release(name_val);
        runtime_error(vm, "Invalid ADT constructor parameters");
        return VM_RUNTIME_ERROR;
    }
    
    // Create a constructor class
    value_t constructor_class = make_class_with_debug(name_val.as.string, NULL, NULL, vm->current_debug);
    
    // Set the factory function
    constructor_class.as.class->factory = adt_constructor_wrapper;
    
    vm_push(vm, constructor_class);
    
    // Clean up
    if (param_names) {
        for (size_t i = 0; i < param_count; i++) {
            free(param_names[i]);
        }
        free(param_names);
    }
    vm_release(param_count_val);
    vm_release(case_type_val);
    vm_release(name_val);
    
    return VM_OK;
}