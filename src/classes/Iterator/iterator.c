#include "iterator.h"
#include "builtins.h"
#include "dynamic_array.h"
#include "dynamic_object.h"

// Using centralized call_equals_method from vm/utilities.c

// Global Iterator class storage
value_t* global_iterator_class = NULL;

// Initialize Iterator class with prototype and methods
void iterator_class_init(vm_t* vm) {
    // Create the Iterator class with its prototype
    do_object iterator_proto = do_create(NULL);

    // Add methods to Iterator prototype
    value_t iterator_has_next_method = make_native(builtin_has_next);
    do_set(iterator_proto, "hasNext", &iterator_has_next_method, sizeof(value_t));

    value_t iterator_next_method = make_native(builtin_next);
    do_set(iterator_proto, "next", &iterator_next_method, sizeof(value_t));

    value_t iterator_is_empty_method = make_native(builtin_iterator_is_empty);
    do_set(iterator_proto, "isEmpty", &iterator_is_empty_method, sizeof(value_t));

    value_t iterator_to_array_method = make_native(builtin_iterator_to_array);
    do_set(iterator_proto, "toArray", &iterator_to_array_method, sizeof(value_t));

    value_t iterator_hash_method = make_native(builtin_iterator_hash);
    do_set(iterator_proto, "hash", &iterator_hash_method, sizeof(value_t));

    value_t iterator_equals_method = make_native(builtin_iterator_equals);
    do_set(iterator_proto, "equals", &iterator_equals_method, sizeof(value_t));

    // Create the Iterator class
    value_t iterator_class = make_class("Iterator", iterator_proto);

    // Store in globals
    do_set(vm->globals, "Iterator", &iterator_class, sizeof(value_t));

    // Store a global reference for use in make_iterator
    static value_t iterator_class_storage;
    iterator_class_storage = vm_retain(iterator_class);
    global_iterator_class = &iterator_class_storage;
}

// iterator(collection) - Create iterator for arrays and ranges
value_t builtin_iterator(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "iterator() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t collection = args[0];
    iterator_t* iter = NULL;

    switch (collection.type) {
    case VAL_ARRAY:
        iter = create_array_iterator(collection.as.array);
        break;
    case VAL_RANGE:
        if (collection.as.range) {
            iter = create_range_iterator(collection.as.range->start, collection.as.range->end,
                                         collection.as.range->exclusive, collection.as.range->step);
        }
        break;
    default:
        runtime_error(vm, "iterator() can only be called on arrays and ranges, not %s", value_type_name(collection.type));
    }

    if (!iter) {
        runtime_error(vm, "Failed to create iterator");
    }

    return make_iterator(iter);
}

// hasNext(iterator) - Check if iterator has more elements
value_t builtin_has_next(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hasNext() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error(vm, "hasNext() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }

    int has_next = iterator_has_next(iter_val.as.iterator);
    return make_boolean(has_next);
}

// next(iterator) - Get next element from iterator
value_t builtin_next(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "next() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t iter_val = args[0];
    if (iter_val.type != VAL_ITERATOR) {
        runtime_error(vm, "next() requires an iterator argument, not %s", value_type_name(iter_val.type));
    }

    if (!iterator_has_next(iter_val.as.iterator)) {
        runtime_error(vm, "Iterator has no more elements");
    }

    return iterator_next(iter_val.as.iterator);
}

// iterator.isEmpty() - Check if iterator has no more elements
value_t builtin_iterator_is_empty(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "isEmpty() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ITERATOR) {
        runtime_error(vm, "isEmpty() can only be called on iterators");
    }
    
    iterator_t* iter = receiver.as.iterator;
    if (!iter) {
        runtime_error(vm, "Invalid iterator");
    }
    
    int has_next = iterator_has_next(iter);
    return make_boolean(!has_next); // isEmpty is !hasNext
}

// iterator.toArray() - Consume remaining elements into an array
value_t builtin_iterator_to_array(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "toArray() takes no arguments (%d given)", arg_count - 1);
    }
    
    value_t receiver = args[0];
    if (receiver.type != VAL_ITERATOR) {
        runtime_error(vm, "toArray() can only be called on iterators");
    }
    
    iterator_t* iter = receiver.as.iterator;
    if (!iter) {
        runtime_error(vm, "Invalid iterator");
    }
    
    // Create new array to collect elements
    da_array array = da_new(sizeof(value_t));
    
    // Consume all remaining elements from iterator
    while (iterator_has_next(iter)) {
        value_t element = iterator_next(iter);
        da_push(array, &element);
    }
    
    return make_array(array);
}

// Forward declaration for the global value hash function
extern value_t builtin_value_hash(vm_t* vm, int arg_count, value_t* args);

// iterator.hash() - Compute hash based on iterator state and type
value_t builtin_iterator_hash(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "hash() takes no arguments (%d given)", arg_count - 1);
        return make_null();
    }
    
    value_t receiver = args[0];
    
    if (receiver.type != VAL_ITERATOR) {
        runtime_error(vm, "hash() can only be called on iterators");
        return make_null();
    }
    
    iterator_t* iter = receiver.as.iterator;
    
    // Hash based on iterator type and state
    uint32_t type_hash = (uint32_t)iter->type;
    uint32_t combined = type_hash;
    
    // Add specific state based on iterator type
    if (iter->type == ITER_ARRAY) {
        // Hash based on array contents and current index
        value_t array_value = make_array(iter->data.array_iter.array);
        value_t array_hash_result = builtin_value_hash(vm, 1, &array_value);
        if (array_hash_result.type == VAL_INT32) {
            combined ^= (uint32_t)array_hash_result.as.int32;
        }
        combined ^= (uint32_t)iter->data.array_iter.index << 8;
        vm_release(array_value);
    } else if (iter->type == ITER_RANGE) {
        // Hash based on current position and range parameters
        value_t current_hash = builtin_value_hash(vm, 1, &iter->data.range_iter.current);
        value_t end_hash = builtin_value_hash(vm, 1, &iter->data.range_iter.end);
        if (current_hash.type == VAL_INT32 && end_hash.type == VAL_INT32) {
            combined ^= (uint32_t)current_hash.as.int32;
            combined ^= (uint32_t)end_hash.as.int32 << 4;
        }
        combined ^= iter->data.range_iter.exclusive ? (1 << 12) : 0;
        combined ^= iter->data.range_iter.finished ? (1 << 13) : 0;
    }
    
    return make_int32((int32_t)combined);
}

// iterator.equals(other) - Compare iterator state and type
value_t builtin_iterator_equals(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "equals() takes exactly 1 argument (%d given)", arg_count - 1);
        return make_null();
    }
    
    value_t receiver = args[0];
    value_t other = args[1];
    
    if (receiver.type != VAL_ITERATOR) {
        runtime_error(vm, "equals() can only be called on iterators");
        return make_null();
    }
    
    // Iterator only equals another iterator
    if (other.type != VAL_ITERATOR) {
        return make_boolean(0);
    }
    
    iterator_t* iter1 = receiver.as.iterator;
    iterator_t* iter2 = other.as.iterator;
    
    // Must be same type
    if (iter1->type != iter2->type) {
        return make_boolean(0);
    }
    
    // Compare state based on iterator type
    if (iter1->type == ITER_ARRAY) {
        // Compare array contents and current position
        if (iter1->data.array_iter.index != iter2->data.array_iter.index) {
            return make_boolean(0);
        }
        // Compare arrays element by element
        da_array arr1 = iter1->data.array_iter.array;
        da_array arr2 = iter2->data.array_iter.array;
        if (da_length(arr1) != da_length(arr2)) {
            return make_boolean(0);
        }
        for (size_t i = 0; i < da_length(arr1); i++) {
            value_t* elem1 = (value_t*)da_get(arr1, i);
            value_t* elem2 = (value_t*)da_get(arr2, i);
            if (!call_equals_method(vm, *elem1, *elem2)) {
                return make_boolean(0);
            }
        }
        return make_boolean(1);
    } else if (iter1->type == ITER_RANGE) {
        // Compare all range iterator state
        if (iter1->data.range_iter.exclusive != iter2->data.range_iter.exclusive ||
            iter1->data.range_iter.finished != iter2->data.range_iter.finished ||
            iter1->data.range_iter.reverse != iter2->data.range_iter.reverse) {
            return make_boolean(0);
        }
        // Compare values
        int current_equal = call_equals_method(vm, iter1->data.range_iter.current, iter2->data.range_iter.current);
        int end_equal = call_equals_method(vm, iter1->data.range_iter.end, iter2->data.range_iter.end);
        int step_equal = call_equals_method(vm, iter1->data.range_iter.step, iter2->data.range_iter.step);
        
        return make_boolean(current_equal && end_equal && step_equal);
    }
    
    return make_boolean(0);
}