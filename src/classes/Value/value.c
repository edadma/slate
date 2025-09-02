#include "value.h"
#include "builtins.h"
#include "dynamic_string.h"
#include "dynamic_array.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Forward declarations for functions still in builtins.c
value_t builtin_local_date_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_local_time_to_string(vm_t* vm, int arg_count, value_t* args);
value_t builtin_buffer_method_to_string(vm_t* vm, int arg_count, value_t* args);

// builtin_type() - Get type name of any value
value_t builtin_type(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "type() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t arg = args[0];
    const char* type_name;

    switch (arg.type) {
    case VAL_INT32:
        type_name = "int32";
        break;
    case VAL_BIGINT:
        type_name = "bigint";
        break;
    case VAL_FLOAT32:
        type_name = "float32";
        break;
    case VAL_FLOAT64:
        type_name = "float64";
        break;
    case VAL_STRING:
        type_name = "string";
        break;
    case VAL_BOOLEAN:
        type_name = "boolean";
        break;
    case VAL_NULL:
        type_name = "null";
        break;
    case VAL_UNDEFINED:
        type_name = "undefined";
        break;
    case VAL_ARRAY:
        type_name = "array";
        break;
    case VAL_OBJECT:
        type_name = "object";
        break;
    case VAL_CLASS:
        type_name = "class";
        break;
    case VAL_FUNCTION:
        type_name = "function";
        break;
    case VAL_CLOSURE:
        type_name = "closure";
        break;
    case VAL_NATIVE:
        type_name = "builtin";
        break;
    case VAL_RANGE:
        type_name = "range";
        break;
    case VAL_ITERATOR:
        type_name = "iterator";
        break;
    case VAL_BUFFER:
        type_name = "buffer";
        break;
    case VAL_BUFFER_BUILDER:
        type_name = "buffer_builder";
        break;
    case VAL_BUFFER_READER:
        type_name = "buffer_reader";
        break;
    case VAL_BOUND_METHOD:
        type_name = "bound_method";
        break;
    case VAL_LOCAL_DATE:
        type_name = "LocalDate";
        break;
    case VAL_LOCAL_TIME:
        type_name = "LocalTime";
        break;
    case VAL_LOCAL_DATETIME:
        type_name = "LocalDateTime";
        break;
    case VAL_ZONE:
        type_name = "Zone";
        break;
    case VAL_DATE:
        type_name = "Date";
        break;
    case VAL_INSTANT:
        type_name = "Instant";
        break;
    case VAL_DURATION:
        type_name = "Duration";
        break;
    case VAL_PERIOD:
        type_name = "Period";
        break;
    default:
        type_name = "unknown";
        break;
    }

    return make_string(type_name);
}

// builtin_value_to_string() - Convert any value to string representation
value_t builtin_value_to_string(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toString() takes no arguments (%d given)", arg_count);
    }
    
    value_t receiver = args[0];
    
    switch (receiver.type) {
        case VAL_NULL:
            return make_string("null");
            
        case VAL_UNDEFINED:
            return make_string("undefined");
            
        case VAL_BOOLEAN:
            return make_string(receiver.as.boolean ? "true" : "false");
            
        case VAL_INT32:
            {
                char buffer[32];
                snprintf(buffer, sizeof(buffer), "%d", receiver.as.int32);
                return make_string(buffer);
            }
            
        case VAL_BIGINT:
            {
                char* str = di_to_string(receiver.as.bigint, 10);
                value_t result = make_string(str);
                free(str);
                return result;
            }
            
        case VAL_FLOAT32:
            {
                float val = receiver.as.float32;
                if (isnan(val)) {
                    return make_string("NaN");
                } else if (isinf(val)) {
                    return make_string(val > 0 ? "Infinity" : "-Infinity");
                } else {
                    char buffer[32];
                    // Use %.7g for clean formatting without unnecessary zeros (float32 precision)
                    snprintf(buffer, sizeof(buffer), "%.7g", val);
                    return make_string(buffer);
                }
            }
            
        case VAL_FLOAT64:
            {
                double val = receiver.as.float64;
                if (isnan(val)) {
                    return make_string("NaN");
                } else if (isinf(val)) {
                    return make_string(val > 0 ? "Infinity" : "-Infinity");
                } else {
                    char buffer[64];
                    // Use %.15g for clean formatting without unnecessary zeros
                    snprintf(buffer, sizeof(buffer), "%.15g", val);
                    return make_string(buffer);
                }
            }
            
        case VAL_STRING:
            // Strings return themselves (no quotes)
            return make_string(receiver.as.string);
            
        case VAL_ARRAY:
            {
                // Format as "[1, 2, 3]"
                ds_builder sb = ds_builder_create();
                ds_string bracket_open = ds_new("[");
                ds_builder_append_string(sb, bracket_open);
                ds_release(&bracket_open);
                
                da_array array = receiver.as.array;
                size_t count = da_length(array);
                
                for (size_t i = 0; i < count; i++) {
                    if (i > 0) {
                        ds_string separator = ds_new(", ");
                        ds_builder_append_string(sb, separator);
                        ds_release(&separator);
                    }
                    
                    value_t* element = (value_t*)da_get(array, i);
                    // Recursively call toString on each element
                    value_t element_str = builtin_value_to_string(vm, 1, element);
                    ds_builder_append_string(sb, element_str.as.string);
                    vm_release(element_str);
                }
                
                ds_string bracket_close = ds_new("]");
                ds_builder_append_string(sb, bracket_close);
                ds_release(&bracket_close);
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_OBJECT:
            {
                // Format as "{key: value, key2: value2}"
                ds_builder sb = ds_builder_create();
                ds_string brace_open = ds_new("{");
                ds_builder_append_string(sb, brace_open);
                ds_release(&brace_open);
                
                // TODO: Implement object iteration when available
                // For now, return a placeholder
                ds_string placeholder = ds_new("...}");
                ds_builder_append_string(sb, placeholder);
                ds_release(&placeholder);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_STRING_BUILDER:
            {
                // Convert StringBuilder to string and format as "StringBuilder(...)"
                ds_string content = ds_builder_to_string(receiver.as.string_builder);
                ds_builder sb = ds_builder_create();
                ds_string prefix = ds_new("StringBuilder(\"");
                ds_builder_append_string(sb, prefix);
                ds_release(&prefix);
                ds_builder_append_string(sb, content);
                ds_string suffix = ds_new("\")");
                ds_builder_append_string(sb, suffix);
                ds_release(&suffix);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                ds_release(&content);
                
                return make_string_ds(result);
            }
            
        case VAL_LOCAL_DATE:
            {
                // Use existing LocalDate toString functionality
                return builtin_local_date_to_string(vm, arg_count, args);
            }
            
        case VAL_LOCAL_TIME:
            {
                // Use existing LocalTime toString functionality  
                return builtin_local_time_to_string(vm, arg_count, args);
            }
            
        case VAL_BUFFER:
            {
                // Convert buffer to hex representation
                return builtin_buffer_method_to_string(vm, arg_count, args);
            }
            
        case VAL_BUFFER_READER:
            return make_string("BufferReader");
            
        case VAL_RANGE:
            {
                // Format as "1..10" or "1..<10"
                range_t* range = receiver.as.range;
                ds_builder sb = ds_builder_create();
                
                // Convert start to string
                // Get the start value directly (already a value_t)
                value_t start_val = range->start;
                value_t start_str = builtin_value_to_string(vm, 1, &start_val);
                ds_builder_append_string(sb, start_str.as.string);
                vm_release(start_str);
                
                // Add range operator
                if (range->exclusive) {
                    ds_string range_op = ds_new("..<");
                    ds_builder_append_string(sb, range_op);
                    ds_release(&range_op);
                } else {
                    ds_string range_op = ds_new("..");
                    ds_builder_append_string(sb, range_op);
                    ds_release(&range_op);
                }
                
                // Convert end to string
                // Get the end value directly (already a value_t)  
                value_t end_val = range->end;
                value_t end_str = builtin_value_to_string(vm, 1, &end_val);
                ds_builder_append_string(sb, end_str.as.string);
                vm_release(end_str);
                
                ds_string result = ds_builder_to_string(sb);
                ds_builder_release(&sb);
                
                return make_string_ds(result);
            }
            
        case VAL_ITERATOR:
            return make_string("Iterator");
            
        case VAL_NATIVE:
            return make_string("native function");
            
        case VAL_BOUND_METHOD:
            return make_string("bound method");
            
        default:
            return make_string("unknown");
    }
}