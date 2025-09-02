#include "iterator.h"
#include "builtins.h"
#include "dynamic_array.h"
#include "dynamic_object.h"

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
                                         collection.as.range->exclusive);
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