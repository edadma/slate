#include "range.h"
#include "builtins.h"
#include "vm.h"
#include "dynamic_array.h"
#include "dynamic_object.h"

// Global Range class storage
value_t* global_range_class = NULL;

// Initialize Range class with prototype and methods
void range_class_init(vm_t* vm) {
    // Create the Range class with its prototype
    do_object range_proto = do_create(NULL);

    // Add methods to Range prototype
    value_t range_iterator_method = make_native(builtin_iterator);
    do_set(range_proto, "iterator", &range_iterator_method, sizeof(value_t));

    value_t range_start_method = make_native(builtin_range_start);
    do_set(range_proto, "start", &range_start_method, sizeof(value_t));

    value_t range_end_method = make_native(builtin_range_end);
    do_set(range_proto, "endValue", &range_end_method, sizeof(value_t));

    value_t range_is_exclusive_method = make_native(builtin_range_is_exclusive);
    do_set(range_proto, "isExclusive", &range_is_exclusive_method, sizeof(value_t));

    value_t range_is_empty_method = make_native(builtin_range_is_empty);
    do_set(range_proto, "isEmpty", &range_is_empty_method, sizeof(value_t));

    value_t range_length_method = make_native(builtin_range_length);
    do_set(range_proto, "length", &range_length_method, sizeof(value_t));

    value_t range_contains_method = make_native(builtin_range_contains);
    do_set(range_proto, "contains", &range_contains_method, sizeof(value_t));

    value_t range_to_array_method = make_native(builtin_range_to_array);
    do_set(range_proto, "toArray", &range_to_array_method, sizeof(value_t));

    value_t range_reverse_method = make_native(builtin_range_reverse);
    do_set(range_proto, "reverse", &range_reverse_method, sizeof(value_t));

    value_t range_equals_method = make_native(builtin_range_equals);
    do_set(range_proto, "equals", &range_equals_method, sizeof(value_t));

    // Create the Range class
    value_t range_class = make_class("Range", range_proto);

    // Store in globals
    do_set(vm->globals, "Range", &range_class, sizeof(value_t));

    // Store a global reference for use in make_range
    static value_t range_class_storage;
    range_class_storage = vm_retain(range_class);
    global_range_class = &range_class_storage;
}

// range.start() - Get the starting value
value_t builtin_range_start(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "start() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "start() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    return vm_retain(range->start);
}

// range.end() - Get the ending value
value_t builtin_range_end(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "end() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "end() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    return vm_retain(range->end);
}

// range.isExclusive() - Check if range excludes end value
value_t builtin_range_is_exclusive(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isExclusive() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "isExclusive() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    return make_boolean(range->exclusive);
}

// range.isEmpty() - Check if range contains no elements
value_t builtin_range_is_empty(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "isEmpty() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    // Range is empty if start > end, or if start == end and exclusive
    if (!is_number(range->start) || !is_number(range->end)) {
        // Non-numeric ranges are not empty by default
        return make_boolean(0);
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    
    if (start_val > end_val) {
        return make_boolean(1); // Empty - start > end
    }
    
    if (start_val == end_val && range->exclusive) {
        return make_boolean(1); // Empty - start == end with exclusive
    }
    
    return make_boolean(0); // Not empty
}

// range.length() - Number of elements in range
value_t builtin_range_length(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "length() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "length() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    // Only support numeric ranges for length calculation
    if (!is_number(range->start) || !is_number(range->end)) {
        runtime_error(vm, "length() only supported for numeric ranges");
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    
    if (start_val > end_val) {
        return make_int32(0); // Empty range
    }
    
    if (start_val == end_val) {
        return make_int32(range->exclusive ? 0 : 1);
    }
    
    // Calculate length based on integer step
    int start_int = (int)start_val;
    int end_int = (int)end_val;
    
    if (range->exclusive) {
        return make_int32(end_int - start_int);
    } else {
        return make_int32(end_int - start_int + 1);
    }
}

// range.contains(value) - Check if value is within range bounds
value_t builtin_range_contains(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "contains() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t value = args[1];
    
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "contains() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    // Only support numeric values for contains check
    if (!is_number(range->start) || !is_number(range->end) || !is_number(value)) {
        return make_boolean(0); // Non-numeric not contained
    }
    
    double start_val = value_to_double(range->start);
    double end_val = value_to_double(range->end);
    double check_val = value_to_double(value);
    
    if (range->exclusive) {
        return make_boolean(check_val >= start_val && check_val < end_val);
    } else {
        return make_boolean(check_val >= start_val && check_val <= end_val);
    }
}

// range.toArray() - Convert range to array
value_t builtin_range_to_array(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toArray() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "toArray() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    // Only support numeric ranges for conversion to array
    if (!is_number(range->start) || !is_number(range->end)) {
        runtime_error(vm, "toArray() only supported for numeric ranges");
    }
    
    // Use iterator pattern for consistent behavior with Iterator.toArray()
    // Create iterator for this range
    iterator_t* iter = create_range_iterator(range->start, range->end, range->exclusive);
    if (!iter) {
        runtime_error(vm, "Failed to create range iterator");
    }
    
    // Create new array to collect elements
    da_array array = da_new(sizeof(value_t));
    
    // Consume all elements from iterator (handles forward/reverse automatically)
    while (iterator_has_next(iter)) {
        value_t element = iterator_next(iter);
        da_push(array, &element);
    }
    
    // Clean up iterator
    iterator_release(iter);
    
    return make_array(array);
}

// range.reverse() - Create reversed range
value_t builtin_range_reverse(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reverse() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "reverse() can only be called on ranges");
    }
    
    range_t* range = receiver.as.range;
    if (!range) {
        runtime_error(vm, "Invalid range");
    }
    
    // Create reversed range: swap start and end, keep exclusivity
    return make_range(range->end, range->start, range->exclusive);
}

// range.equals(other) - Deep equality comparison
value_t builtin_range_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_RANGE) {
        runtime_error(vm, "equals() can only be called on ranges");
    }
    
    if (other.type != VAL_RANGE) {
        return make_boolean(0); // Different types not equal
    }
    
    range_t* range1 = receiver.as.range;
    range_t* range2 = other.as.range;
    
    if (!range1 || !range2) {
        return make_boolean(0); // Invalid ranges not equal
    }
    
    // Compare exclusivity flag
    if (range1->exclusive != range2->exclusive) {
        return make_boolean(0);
    }
    
    // Compare start and end values
    int start_equal = values_equal(range1->start, range2->start);
    int end_equal = values_equal(range1->end, range2->end);
    
    return make_boolean(start_equal && end_equal);
}