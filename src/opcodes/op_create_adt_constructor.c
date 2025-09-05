#include "vm.h"
#include "runtime_error.h"
#include "ast.h"
#include "../classes/Object/class_object.h"
#include "../classes/ADT/adt_methods.h"
#include "dynamic_string.h"
#include "dynamic_array.h"

// ADT method implementations are now in src/classes/ADT/adt_methods.c

// Wrapper factory function that creates ADT instances
static value_t adt_constructor_wrapper(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    // Create a basic ADT instance  
    do_object object = do_create(NULL);
    if (!object) {
        return make_null();
    }
    
    value_t instance = make_object_with_debug(object, vm->current_debug);
    
    // Get constructor information directly from self - no more complex lookups needed!
    const char* constructor_name = self->name; // "Some", "None", "b", etc.
    
    // Get metadata from the constructor class's static properties  
    ds_string* case_type_metadata = NULL;
    if (self->static_properties) {
        case_type_metadata = (ds_string*)do_get(self->static_properties, "__constructor_case_type");
    }
    
    const char* case_type_str = "constructor"; // default
    if (case_type_metadata) {
        case_type_str = *case_type_metadata;
    }
    
    // No need to find ADT base class - each constructor will have its own methods
    
    // Set the constructor class as the instance's class (same as any factory)
    value_t* class_value = malloc(sizeof(value_t));
    if (class_value) {
        *class_value = (value_t){.type = VAL_CLASS, .as.class = self};
        instance.class = class_value;
    }
    
    // No need to store metadata in instance - it's all in the class now
    
    // Store constructor arguments using parameter names from class metadata
    da_array* param_names_array = (da_array*)do_get(self->static_properties, "__params__");
    if (param_names_array && arg_count > 0) {
        // Use actual parameter names from class metadata
        for (int i = 0; i < arg_count && i < (int)da_length(*param_names_array); i++) {
            ds_string* param_name_ptr = (ds_string*)da_get(*param_names_array, i);
            if (param_name_ptr) {
                value_t* arg_copy = malloc(sizeof(value_t));
                if (arg_copy) {
                    *arg_copy = vm_retain(args[i]);
                    do_set(instance.as.object, *param_name_ptr, arg_copy, sizeof(value_t));
                }
            }
        }
    } else if (arg_count > 0) {
        // Fallback to numbered parameters if metadata not available
        for (int i = 0; i < arg_count; i++) {
            char param_name[32];
            snprintf(param_name, sizeof(param_name), "param_%d", i);
            value_t* arg_copy = malloc(sizeof(value_t));
            if (arg_copy) {
                *arg_copy = vm_retain(args[i]);
                do_set(instance.as.object, param_name, arg_copy, sizeof(value_t));
            }
        }
    }
    
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
    
    // Store constructor metadata in the class static properties
    if (constructor_class.as.class) {
        if (!constructor_class.as.class->static_properties) {
            constructor_class.as.class->static_properties = do_create(NULL);
        }
        
        // Store case name
        ds_string case_name_str = ds_new(name_val.as.string);
        do_set(constructor_class.as.class->static_properties, "__constructor_case_name", &case_name_str, sizeof(ds_string));
        
        // Store case type based on parameter count - if it has params, it's a constructor
        ds_string case_type_str = ds_new((param_count > 0) ? "constructor" : "singleton");
        do_set(constructor_class.as.class->static_properties, "__constructor_case_type", &case_type_str, sizeof(ds_string));
        
        // Store parameter count
        int32_t* param_count_ptr = malloc(sizeof(int32_t));
        if (param_count_ptr) {
            *param_count_ptr = param_count_val.as.int32;
            do_set(constructor_class.as.class->static_properties, "__constructor_param_count", param_count_ptr, sizeof(int32_t));
        }
        
        // Store parameter names as an array for easy access
        if (param_count > 0) {
            // Create array of parameter names with proper ds_string management
            da_array param_names_array = da_create(sizeof(ds_string), 0, 
                                                   (void(*)(void*))ds_retain,
                                                   (void(*)(void*))ds_release);
            for (size_t i = 0; i < param_count; i++) {
                ds_string param_name_str = ds_new(param_names[i]);
                da_push(param_names_array, &param_name_str);
            }
            do_set(constructor_class.as.class->static_properties, "__params__", &param_names_array, sizeof(da_array));
        }
        
        // Add static methods to constructor class
        value_t class_toString_method = make_native(adt_class_toString);
        do_set(constructor_class.as.class->static_properties, "toString", &class_toString_method, sizeof(value_t));
        
        value_t class_equals_method = make_native(adt_class_equals);
        do_set(constructor_class.as.class->static_properties, "equals", &class_equals_method, sizeof(value_t));
        
        value_t class_hash_method = make_native(adt_class_hash);
        do_set(constructor_class.as.class->static_properties, "hash", &class_hash_method, sizeof(value_t));
        
        // Add ADT instance methods to constructor class
        if (!constructor_class.as.class->instance_properties) {
            constructor_class.as.class->instance_properties = do_create(NULL);
        }
        
        value_t instance_toString_method = make_native(adt_instance_toString);
        do_set(constructor_class.as.class->instance_properties, "toString", &instance_toString_method, sizeof(value_t));
        
        value_t instance_equals_method = make_native(adt_instance_equals);
        do_set(constructor_class.as.class->instance_properties, "equals", &instance_equals_method, sizeof(value_t));
        
        value_t instance_hash_method = make_native(adt_instance_hash);
        do_set(constructor_class.as.class->instance_properties, "hash", &instance_hash_method, sizeof(value_t));
    }
    
    // Create a specialized factory function for this specific constructor
    // For now, we'll use a shared factory and store the constructor name in the class
    // The factory will extract the name when creating instances
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