#include "class_object.h"
#include "builtins.h"
#include "dynamic_object.h"
#include <stb_ds.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

// FNV-1a hash constants for 32-bit
#define FNV_32_PRIME 0x01000193
#define FNV_32_OFFSET_BASIS 0x811c9dc5

// Forward declaration for the global value hash function
extern value_t builtin_value_hash(vm_t* vm, int arg_count, value_t* args);

// Forward declaration for string hash function
extern value_t builtin_string_hash(vm_t* vm, int arg_count, value_t* args);

// Helper function to get hash code for object property values (avoids recursion)
static uint32_t hash_object_property_value(vm_t* vm, value_t value) {
    // For objects, use pointer identity to avoid infinite recursion
    if (value.type == VAL_OBJECT) {
        return (uint32_t)(uintptr_t)value.as.object;
    }
    
    // For all other types, use the global hash function
    value_t hash_result = builtin_value_hash(vm, 1, &value);
    return (uint32_t)hash_result.as.int32;
}

// Helper function to hash a string key using existing string hash
static uint32_t hash_string_key(vm_t* vm, const char* key) {
    value_t key_string = make_string(key);
    value_t hash_result = builtin_string_hash(vm, 1, &key_string);
    uint32_t result = (uint32_t)hash_result.as.int32;
    vm_release(key_string);
    return result;
}

// Object method: hash() - Content-based hash using key-value pairs
value_t builtin_object_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "hash() can only be called on objects");
    }
    
    uint32_t hash = FNV_32_OFFSET_BASIS;
    
    // Get all keys in the object
    const char** keys = do_get_own_keys(receiver.as.object);
    if (keys) {
        // Get key count using stb_ds
        int count = arrlen(keys);
        
        // Sort keys for consistent hashing (simple bubble sort for small objects)
        for (int i = 0; i < count - 1; i++) {
            for (int j = 0; j < count - i - 1; j++) {
                if (strcmp(keys[j], keys[j + 1]) > 0) {
                    const char* temp = keys[j];
                    keys[j] = keys[j + 1];
                    keys[j + 1] = temp;
                }
            }
        }
        
        // Hash each key-value pair in sorted order
        for (int i = 0; i < count; i++) {
            const char* key = keys[i];
            
            // Hash the key using string hash function
            uint32_t key_hash = hash_string_key(vm, key);
            hash ^= key_hash;
            hash *= FNV_32_PRIME;
            
            // Hash the value
            value_t* val_ptr = (value_t*)do_get(receiver.as.object, key);
            if (val_ptr) {
                uint32_t val_hash = hash_object_property_value(vm, *val_ptr);
                hash ^= val_hash;
                hash *= FNV_32_PRIME;
            }
        }
        
        arrfree(keys);
    }
    
    return make_int32((int32_t)hash);
}

// Object method: equals(other) - Deep equality comparison for objects
value_t builtin_object_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "equals() can only be called on objects");
    }
    
    // Only objects can be equal to objects
    if (other.type != VAL_OBJECT) {
        return make_boolean(0);
    }
    
    // Identity equality check
    if (receiver.as.object == other.as.object) {
        return make_boolean(1);
    }
    
    // Handle null objects
    if (receiver.as.object == NULL && other.as.object == NULL) {
        return make_boolean(1);
    }
    if (receiver.as.object == NULL || other.as.object == NULL) {
        return make_boolean(0);
    }
    
    // Get keys from both objects
    const char** keys1 = do_get_own_keys(receiver.as.object);
    const char** keys2 = do_get_own_keys(other.as.object);
    
    // Compare key counts
    size_t count1 = arrlen(keys1);
    size_t count2 = arrlen(keys2);
    
    if (count1 != count2) {
        arrfree(keys1);
        arrfree(keys2);
        return make_boolean(0);
    }
    
    // Sort keys for consistent comparison (using simple bubble sort)
    for (size_t i = 0; i < count1 - 1; i++) {
        for (size_t j = 0; j < count1 - 1 - i; j++) {
            if (strcmp(keys1[j], keys1[j + 1]) > 0) {
                const char* temp = keys1[j];
                keys1[j] = keys1[j + 1];
                keys1[j + 1] = temp;
            }
        }
    }
    
    for (size_t i = 0; i < count2 - 1; i++) {
        for (size_t j = 0; j < count2 - 1 - i; j++) {
            if (strcmp(keys2[j], keys2[j + 1]) > 0) {
                const char* temp = keys2[j];
                keys2[j] = keys2[j + 1];
                keys2[j + 1] = temp;
            }
        }
    }
    
    // Compare sorted keys and their values
    for (size_t i = 0; i < count1; i++) {
        // Check key equality
        if (strcmp(keys1[i], keys2[i]) != 0) {
            arrfree(keys1);
            arrfree(keys2);
            return make_boolean(0);
        }
        
        // Check value equality
        value_t* val1 = (value_t*)do_get(receiver.as.object, keys1[i]);
        value_t* val2 = (value_t*)do_get(other.as.object, keys2[i]);
        
        if (val1 == NULL && val2 == NULL) continue;
        if (val1 == NULL || val2 == NULL) {
            arrfree(keys1);
            arrfree(keys2);
            return make_boolean(0);
        }
        
        // Use method dispatch for deep value comparison
        if (!call_equals_method(vm, *val1, *val2)) {
            arrfree(keys1);
            arrfree(keys2);
            return make_boolean(0);
        }
    }
    
    arrfree(keys1);
    arrfree(keys2);
    return make_boolean(1);
}

// Object method: toString()
value_t builtin_object_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_OBJECT) {
        runtime_error(vm, "toString() can only be called on objects");
    }
    
    // Check if this is an ADT object with type information
    ds_string* type_name = (ds_string*)do_get(receiver.as.object, "__type");
    ds_string* case_type = (ds_string*)do_get(receiver.as.object, "__case_type");
    
    if (type_name && case_type) {
        if (strcmp(*case_type, "singleton") == 0) {
            // Singleton case: just return the case name
            return make_string(*type_name);
        } else if (strcmp(*case_type, "constructor") == 0) {
            // Constructor case: show case name with parameters
            // For now, show the case name - could be enhanced to show parameter values
            char buffer[256];
            snprintf(buffer, sizeof(buffer), "%s(...)", *type_name);
            return make_string(buffer);
        }
    }
    
    // Fallback to generic object representation showing actual properties
    // Create string representation like {key: value, key2: value2}
    ds_string result = ds_new("{");
    
    if (receiver.as.object) {
        const char** keys = do_get_own_keys(receiver.as.object);
        size_t key_count = arrlen(keys);
        int property_count = 0;
        
        for (size_t i = 0; i < key_count; i++) {
            // Skip ADT metadata keys
            if (strstr(keys[i], "__") == keys[i]) {
                continue;
            }
            
            // Add comma separator for subsequent properties
            if (property_count > 0) {
                ds_string comma = ds_new(", ");
                ds_string temp = ds_concat(result, comma);
                ds_release(&result);
                ds_release(&comma);
                result = temp;
            }
            
            // Add key: value
            ds_string key_str = ds_new(keys[i]);
            ds_string colon = ds_new(": ");
            ds_string temp1 = ds_concat(result, key_str);
            ds_string temp2 = ds_concat(temp1, colon);
            ds_release(&result);
            ds_release(&key_str);
            ds_release(&colon);
            ds_release(&temp1);
            
            // Get and convert the value
            value_t* val = (value_t*)do_get(receiver.as.object, keys[i]);
            if (val) {
                ds_string val_str = display_value_to_string(vm, *val);
                ds_string temp3 = ds_concat(temp2, val_str);
                ds_release(&temp2);
                ds_release(&val_str);
                result = temp3;
            } else {
                ds_string unknown = ds_new("?");
                ds_string temp3 = ds_concat(temp2, unknown);
                ds_release(&temp2);
                ds_release(&unknown);
                result = temp3;
            }
            
            property_count++;
        }
        
        arrfree(keys);
    }
    
    ds_string bracket = ds_new("}");
    ds_string temp = ds_concat(result, bracket);
    ds_release(&result);
    ds_release(&bracket);
    
    return make_string_ds(temp);
}

// Stub implementations for other methods
value_t builtin_object_keys(vm_t* vm, int arg_count, value_t* args) {
    runtime_error(vm, "Object.keys() not yet implemented");
}

value_t builtin_object_values(vm_t* vm, int arg_count, value_t* args) {
    runtime_error(vm, "Object.values() not yet implemented");
}

value_t builtin_object_has(vm_t* vm, int arg_count, value_t* args) {
    runtime_error(vm, "Object.has() not yet implemented");
}