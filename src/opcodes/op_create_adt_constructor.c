#include "vm.h"
#include "runtime_error.h"
#include "ast.h"
#include "../classes/Object/class_object.h"
#include "../classes/ADT/adt_methods.h"
#include "dynamic_string.h"

// ADT method implementations are now in src/classes/ADT/adt_methods.c

// Wrapper factory function that creates ADT instances
static value_t adt_constructor_wrapper(vm_t* vm, int arg_count, value_t* args) {
    // Create a basic ADT instance  
    do_object object = do_create(NULL);
    if (!object) {
        return make_null();
    }
    
    value_t instance = make_object_with_debug(object, vm->current_debug);
    
    // We need to set the instance class to the data base class, not Object
    // Look for the ADT base class in globals (should be named after the data type)
    value_t* adt_base_class = NULL;
    const char** global_keys = do_get_own_keys(vm->globals);
    if (global_keys) {
        for (int i = 0; i < (int)arrlen(global_keys); i++) {
            value_t* global_val_ptr = (value_t*)do_get(vm->globals, global_keys[i]);
            if (global_val_ptr && global_val_ptr->type == VAL_CLASS && global_val_ptr->as.class) {
                // Check if this is an ADT base class (has instance properties with toString method)
                if (global_val_ptr->as.class->instance_properties) {
                    value_t* toString_method = (value_t*)do_get(global_val_ptr->as.class->instance_properties, "toString");
                    if (toString_method && toString_method->type == VAL_NATIVE && 
                        toString_method->as.native == adt_instance_toString) {
                        // Found the ADT base class
                        adt_base_class = global_val_ptr;
                        break;
                    }
                }
            }
        }
        arrfree(global_keys);
    }
    
    // Use the ADT base class if found, otherwise fall back to Object class
    if (adt_base_class) {
        instance.class = adt_base_class; // adt_base_class is already a value_t*
    } else {
        instance.class = global_object_class; // global_object_class is also a value_t*
    }
    
    // For now, we'll store a generic type name
    // The toString method will be responsible for displaying the correct constructor name
    const char* constructor_name = "ADTInstance";
    const char* case_type_str = "constructor";
    
    // Set ADT type information 
    ds_string type_name = ds_new(constructor_name);
    ds_string case_type = ds_new(case_type_str);
    do_set(instance.as.object, "__type", &type_name, sizeof(ds_string));
    do_set(instance.as.object, "__case_type", &case_type, sizeof(ds_string));
    
    // TEMPORARY DEBUG: Try a different approach - store the constructor name directly
    // We'll hard-code a simple pattern for now to verify the toString logic works
    const char* temp_constructor_name = "TestConstructor";
    
    // Look through globals to find the constructor class that has this factory
    const char** constructor_keys = do_get_own_keys(vm->globals);
    if (constructor_keys) {
        int key_count = arrlen(constructor_keys);
        for (int i = key_count - 1; i >= 0; i--) {
            value_t* global_val_ptr = (value_t*)do_get(vm->globals, constructor_keys[i]);
            if (global_val_ptr && global_val_ptr->type == VAL_CLASS && global_val_ptr->as.class) {
                // Check if this class has constructor metadata (indicating it's an ADT constructor)
                if (global_val_ptr->as.class->static_properties) {
                    ds_string* case_name = (ds_string*)do_get(global_val_ptr->as.class->static_properties, "__constructor_case_name");
                    if (case_name && global_val_ptr->as.class->factory == adt_constructor_wrapper) {
                        // Found a constructor class with our factory - use its name
                        temp_constructor_name = global_val_ptr->as.class->name;
                        
                        // Store a reference to this class
                        value_t* constructor_class_ref = malloc(sizeof(value_t));
                        if (constructor_class_ref) {
                            *constructor_class_ref = vm_retain(*global_val_ptr);
                            do_set(instance.as.object, "__constructor_class", constructor_class_ref, sizeof(value_t));
                        }
                        break;
                    }
                }
            }
        }
        arrfree(constructor_keys);
    }
    
    // Override the generic type name with the actual constructor name
    ds_string actual_type_name = ds_new(temp_constructor_name);
    do_set(instance.as.object, "__type", &actual_type_name, sizeof(ds_string));
    
    // Store constructor arguments as parameters
    for (int i = 0; i < arg_count; i++) {
        char param_name[32];
        snprintf(param_name, sizeof(param_name), "param_%d", i);
        value_t* arg_copy = malloc(sizeof(value_t));
        if (arg_copy) {
            *arg_copy = vm_retain(args[i]);
            do_set(instance.as.object, param_name, arg_copy, sizeof(value_t));
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
        
        // Store case type  
        ds_string case_type_str = ds_new((case_type_val.as.int32 == 0) ? "singleton" : "constructor");
        do_set(constructor_class.as.class->static_properties, "__constructor_case_type", &case_type_str, sizeof(ds_string));
        
        // Store parameter count
        int32_t* param_count_ptr = malloc(sizeof(int32_t));
        if (param_count_ptr) {
            *param_count_ptr = param_count_val.as.int32;
            do_set(constructor_class.as.class->static_properties, "__constructor_param_count", param_count_ptr, sizeof(int32_t));
        }
        
        // Store parameter names
        for (size_t i = 0; i < param_count; i++) {
            char key[64];
            snprintf(key, sizeof(key), "__constructor_param_name_%zu", i);
            ds_string param_name_str = ds_new(param_names[i]);
            do_set(constructor_class.as.class->static_properties, key, &param_name_str, sizeof(ds_string));
        }
        
        // Add static methods to constructor class
        value_t class_toString_method = make_native(adt_class_toString);
        do_set(constructor_class.as.class->static_properties, "toString", &class_toString_method, sizeof(value_t));
        
        value_t class_equals_method = make_native(adt_class_equals);
        do_set(constructor_class.as.class->static_properties, "equals", &class_equals_method, sizeof(value_t));
        
        value_t class_hash_method = make_native(adt_class_hash);
        do_set(constructor_class.as.class->static_properties, "hash", &class_hash_method, sizeof(value_t));
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