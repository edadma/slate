#include "class_string.h"
#include "builtins.h"
#include "dynamic_string.h"
#include "dynamic_array.h"

// String factory function for creating strings from codepoints
value_t string_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    // Create a string builder for efficient construction
    ds_builder sb = ds_builder_create();
    
    // Case 1: Single array argument containing codepoints
    if (arg_count == 1 && args[0].type == VAL_ARRAY) {
        da_array arr = args[0].as.array;
        size_t len = da_length(arr);
        
        for (size_t i = 0; i < len; i++) {
            value_t* elem = (value_t*)da_get(arr, i);
            
            // Each element must be an integer
            if (elem->type != VAL_INT32 && elem->type != VAL_BIGINT) {
                ds_builder_release(&sb);
                runtime_error(vm, "String() array elements must be integers (codepoints)");
                return make_null(); // Won't reach here due to exit in runtime_error
            }
            
            // Get the codepoint value
            uint32_t codepoint;
            if (elem->type == VAL_INT32) {
                if (elem->as.int32 < 0) {
                    ds_builder_release(&sb);
                    runtime_error(vm, "Codepoint cannot be negative");
                    return make_null();
                }
                codepoint = (uint32_t)elem->as.int32;
            } else {
                // For bigint, convert to uint32_t and check range
                // For now, just get the low 32 bits (simplified)
                ds_builder_release(&sb);
                runtime_error(vm, "BigInt codepoints not yet supported");
                return make_null();
            }
            
            // Validate Unicode codepoint range
            if (codepoint > 0x10FFFF) {
                ds_builder_release(&sb);
                runtime_error(vm, "Invalid Unicode codepoint");
                return make_null();
            }
            
            // Append the codepoint to the builder
            ds_builder_append_char(sb, codepoint);
        }
    }
    // Case 2: Multiple codepoint arguments (variadic)
    else {
        for (int i = 0; i < arg_count; i++) {
            // Each argument must be an integer
            if (args[i].type != VAL_INT32 && args[i].type != VAL_BIGINT) {
                ds_builder_release(&sb);
                runtime_error(vm, "String() arguments must be integers (codepoints)");
                return make_null();
            }
            
            // Get the codepoint value
            uint32_t codepoint;
            if (args[i].type == VAL_INT32) {
                if (args[i].as.int32 < 0) {
                    ds_builder_release(&sb);
                    runtime_error(vm, "Codepoint cannot be negative");
                    return make_null();
                }
                codepoint = (uint32_t)args[i].as.int32;
            } else {
                // For bigint, would need proper conversion
                ds_builder_release(&sb);
                runtime_error(vm, "BigInt codepoints not yet supported");
                return make_null();
            }
            
            // Validate Unicode codepoint range
            if (codepoint > 0x10FFFF) {
                ds_builder_release(&sb);
                runtime_error(vm, "Invalid Unicode codepoint");
                return make_null();
            }
            
            // Append the codepoint to the builder
            ds_builder_append_char(sb, codepoint);
        }
    }
    
    // Convert builder to string
    ds_string result = ds_builder_to_string(sb);
    ds_builder_release(&sb);
    
    // Create and return the value
    return make_string_ds(result);
}