#include "vm.h"
#include <stdio.h>
#include <string.h>
#include "datetime.h"
#include "instant.h"

// Value creation functions with debug info

// Forward declarations
ds_string value_to_string_representation(vm_t* vm, value_t value);
static ds_string display_value_to_string(vm_t* vm, value_t value);

// Context for object property iteration
struct object_string_context {
    ds_string* result_ptr;
    int count;
    vm_t* vm;
};

// Callback function for object property iteration
static void object_property_to_string_callback(const char* key, void* data, size_t size, void* ctx) {
    struct object_string_context* context = (struct object_string_context*)ctx;
    ds_string* result_ptr = context->result_ptr;
    vm_t* vm = context->vm;

    // Add comma separator for subsequent properties
    if (context->count > 0) {
        ds_string comma = ds_new(", ");
        ds_string temp = ds_concat(*result_ptr, comma);
        ds_release(result_ptr);
        ds_release(&comma);
        *result_ptr = temp;
    }

    // Add key: value
    ds_string key_str = ds_new(key);
    ds_string colon = ds_new(": ");
    ds_string temp1 = ds_concat(*result_ptr, key_str);
    ds_string temp2 = ds_concat(temp1, colon);
    ds_release(result_ptr);
    ds_release(&key_str);
    ds_release(&colon);
    ds_release(&temp1);

    // Convert value to string (assuming it's a value_t)
    if (size == sizeof(value_t)) {
        value_t* val = (value_t*)data;
        ds_string val_str = display_value_to_string(vm, *val);
        ds_string temp3 = ds_concat(temp2, val_str);
        ds_release(&temp2);
        ds_release(&val_str);
        *result_ptr = temp3;
    } else {
        ds_string unknown = ds_new("?");
        ds_string temp3 = ds_concat(temp2, unknown);
        ds_release(&temp2);
        ds_release(&unknown);
        *result_ptr = temp3;
    }

    context->count++;
}

// Helper function to convert any value to string representation for concatenation
ds_string value_to_string_representation(vm_t* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING:
        return ds_retain(value.as.string);
    case VAL_INT32: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%d", value.as.int32);
        return ds_new(buffer);
    }
    case VAL_BIGINT: {
        char* str = di_to_string(value.as.bigint, 10);
        if (str) {
            ds_string result = ds_new(str);
            free(str);
            return result;
        } else {
            return ds_new("<bigint>");
        }
    }
    case VAL_NUMBER: {
        char buffer[32];
        snprintf(buffer, sizeof(buffer), "%.6g", value.as.number);
        return ds_new(buffer);
    }
    case VAL_BOOLEAN:
        return ds_new(value.as.boolean ? "true" : "false");
    case VAL_UNDEFINED:
        return ds_new("undefined");
    case VAL_NULL:
        return ds_new("null");
    case VAL_ARRAY: {
        // Create string representation like [1, 2, 3]
        ds_string result = ds_new("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0) {
                    ds_string comma = ds_new(", ");
                    ds_string temp = ds_concat(result, comma);
                    ds_release(&result);
                    ds_release(&comma);
                    result = temp;
                }
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    ds_string element_str = display_value_to_string(vm, *element);
                    ds_string temp = ds_concat(result, element_str);
                    ds_release(&result);
                    ds_release(&element_str);
                    result = temp;
                }
            }
        }
        ds_string bracket = ds_new("]");
        ds_string temp = ds_concat(result, bracket);
        ds_release(&result);
        ds_release(&bracket);
        return temp;
    }
    case VAL_OBJECT: {
        // Create string representation like {key: value, key2: value2}
        ds_string result = ds_new("{");
        if (value.as.object) {
            // Use a context structure to track iteration state
            struct object_string_context context = {&result, 0, vm};

            // Iterate through properties using do_foreach_property
            do_foreach_property(value.as.object, object_property_to_string_callback, &context);
        }
        ds_string bracket = ds_new("}");
        ds_string temp = ds_concat(result, bracket);
        ds_release(&result);
        ds_release(&bracket);
        return temp;
    }
    case VAL_CLASS: {
        if (value.as.class) {
            ds_string prefix = ds_new("<class ");
            ds_string name = ds_new(value.as.class->name ? value.as.class->name : "anonymous");
            ds_string suffix = ds_new(">");

            ds_string temp1 = ds_concat(prefix, name);
            ds_string result = ds_concat(temp1, suffix);

            ds_release(&prefix);
            ds_release(&name);
            ds_release(&suffix);
            ds_release(&temp1);
            return result;
        } else {
            return ds_new("<null class>");
        }
    }
    case VAL_RANGE: {
        if (!value.as.range) {
            return ds_new("{null range}");
        }

        ds_string start_str = value_to_string_representation(vm, value.as.range->start);
        ds_string range_op = ds_new(value.as.range->exclusive ? "..<" : "..");
        ds_string end_str = value_to_string_representation(vm, value.as.range->end);

        // Concatenate: start + range_op + end
        ds_string temp1 = ds_concat(start_str, range_op);
        ds_string result = ds_concat(temp1, end_str);

        // Clean up
        ds_release(&start_str);
        ds_release(&range_op);
        ds_release(&end_str);
        ds_release(&temp1);

        return result;
    }
    case VAL_ITERATOR: {
        if (!value.as.iterator) {
            return ds_new("{null iterator}");
        }

        if (value.as.iterator->type == ITER_ARRAY) {
            return ds_new("{Array Iterator}");
        } else if (value.as.iterator->type == ITER_RANGE) {
            return ds_new("{Range Iterator}");
        } else {
            return ds_new("{Unknown Iterator}");
        }
    }
    case VAL_BOUND_METHOD: {
        if (!value.as.bound_method) {
            return ds_new("{null bound method}");
        }
        return ds_new("{Bound Method}");
    }
    case VAL_FUNCTION:
        return ds_new("{Function}");
    case VAL_CLOSURE:
        return ds_new("{Closure}");
    case VAL_LOCAL_DATE: {
        if (value.as.local_date) {
            char* str = local_date_to_string(vm, value.as.local_date);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalDate>");
    }
    case VAL_LOCAL_TIME: {
        if (value.as.local_time) {
            char* str = local_time_to_string(vm, value.as.local_time);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalTime>");
    }
    case VAL_LOCAL_DATETIME: {
        if (value.as.local_datetime) {
            char* str = local_datetime_to_string(vm, value.as.local_datetime);
            if (str) {
                ds_string result = ds_new(str);
                free(str);
                return result;
            }
        }
        return ds_new("<LocalDateTime>");
    }
    case VAL_ZONED_DATETIME:
        return ds_new("<ZonedDateTime>");  // TODO: implement string conversion
    case VAL_INSTANT: {
        // Use the toString method for proper ISO 8601 formatting
        value_t str_result = instant_to_string(vm, 1, &value);
        if (str_result.type == VAL_STRING) {
            ds_string result = ds_new(str_result.as.string);
            vm_release(str_result);
            return result;
        } else {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "<Instant:%ld>", value.as.instant_millis);
            return ds_new(buffer);
        }
    }
    case VAL_DURATION:
        return ds_new("<Duration>");  // TODO: implement string conversion
    case VAL_PERIOD:
        return ds_new("<Period>");  // TODO: implement string conversion
    default:
        return ds_new("{Unknown}");
    }
}

// Helper function to convert values to display string (with quotes for strings, for use inside aggregates)
static ds_string display_value_to_string(vm_t* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING: {
        // Add quotes around strings for display inside aggregates
        ds_string quoted = ds_new("\"");
        if (value.as.string) {
            ds_string temp1 = ds_concat(quoted, value.as.string);
            ds_release(&quoted);
            quoted = temp1;
        }
        ds_string end_quote = ds_new("\"");
        ds_string temp2 = ds_concat(quoted, end_quote);
        ds_release(&quoted);
        ds_release(&end_quote);
        return temp2;
    }
    default:
        // For all other types, use the regular representation
        return value_to_string_representation(vm, value);
    }
}

// Print function specifically for the print() builtin - shows strings without quotes
void print_for_builtin(vm_t* vm, value_t value) {
    switch (value.type) {
    case VAL_STRING:
        // Print strings without quotes for direct printing
        printf("%s", value.as.string ? value.as.string : "");
        break;
    default: {
        // For aggregates and other types, use the string representation and print it
        ds_string str = value_to_string_representation(vm, value);
        printf("%s", str);
        ds_release(&str);
        break;
    }
    }
}