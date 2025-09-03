#include "vm.h"
#include <stdlib.h>

// Iterator creation and management functions
iterator_t* create_array_iterator(da_array array) {
    iterator_t* iter = malloc(sizeof(iterator_t));
    if (!iter)
        return NULL;

    iter->ref_count = 1; // Initialize reference count
    iter->type = ITER_ARRAY;
    iter->data.array_iter.array = da_retain(array);
    iter->data.array_iter.index = 0;

    return iter;
}

iterator_t* create_range_iterator(value_t start, value_t end, int exclusive, value_t step) {
    iterator_t* iter = malloc(sizeof(iterator_t));
    if (!iter)
        return NULL;

    iter->ref_count = 1; // Initialize reference count
    iter->type = ITER_RANGE;
    iter->data.range_iter.current = vm_retain(start);
    iter->data.range_iter.end = vm_retain(end);
    iter->data.range_iter.step = vm_retain(step);
    iter->data.range_iter.exclusive = exclusive;
    iter->data.range_iter.finished = 0;

    // Determine if this is a reverse range based on step direction
    if (start.type == VAL_INT32 && end.type == VAL_INT32 && step.type == VAL_INT32) {
        int32_t step_val = step.as.int32;
        int32_t start_val = start.as.int32;
        int32_t end_val = end.as.int32;
        
        // Check if step direction matches range direction
        if ((start_val <= end_val && step_val > 0) || (start_val > end_val && step_val < 0)) {
            iter->data.range_iter.reverse = (step_val < 0) ? 1 : 0;
        } else {
            // Step direction doesn't match range direction - this will result in empty range
            iter->data.range_iter.finished = 1;
            iter->data.range_iter.reverse = 0;
        }
    } else {
        iter->data.range_iter.reverse = 0; // Default to forward for non-integer ranges
    }

    return iter;
}

int iterator_has_next(iterator_t* iter) {
    if (!iter)
        return 0;

    switch (iter->type) {
    case ITER_ARRAY:
        return iter->data.array_iter.index < da_length(iter->data.array_iter.array);
    case ITER_RANGE: {
        if (iter->data.range_iter.finished)
            return 0;

        // For now, only support integer ranges
        if (iter->data.range_iter.current.type != VAL_INT32 || iter->data.range_iter.end.type != VAL_INT32) {
            return 0;
        }

        int32_t current = iter->data.range_iter.current.as.int32;
        int32_t end = iter->data.range_iter.end.as.int32;

        if (iter->data.range_iter.reverse) {
            // Reverse range (e.g., 5..1)  
            if (iter->data.range_iter.exclusive) {
                return current > end;
            } else {
                return current >= end;
            }
        } else {
            // Forward range (e.g., 1..5)
            if (iter->data.range_iter.exclusive) {
                return current < end;
            } else {
                return current <= end;
            }
        }
    }
    default:
        return 0;
    }
}

value_t iterator_next(iterator_t* iter) {
    if (!iter || !iterator_has_next(iter)) {
        return make_null();
    }

    switch (iter->type) {
    case ITER_ARRAY: {
        value_t* element = (value_t*)da_get(iter->data.array_iter.array, iter->data.array_iter.index);
        iter->data.array_iter.index++;
        return element ? vm_retain(*element) : make_null();
    }
    case ITER_RANGE: {
        // Return current value and increment/decrement appropriately
        value_t current = vm_retain(iter->data.range_iter.current);

        // Update current by step value (for now, only support integers)
        if (iter->data.range_iter.current.type == VAL_INT32 && iter->data.range_iter.step.type == VAL_INT32) {
            iter->data.range_iter.current.as.int32 += iter->data.range_iter.step.as.int32;
        } else {
            iter->data.range_iter.finished = 1;
        }

        return current;
    }
    default:
        return make_null();
    }
}

// Iterator reference counting functions
iterator_t* iterator_retain(iterator_t* iter) {
    if (!iter)
        return iter;
    iter->ref_count++;
    return iter;
}

void iterator_release(iterator_t* iter) {
    if (!iter)
        return;

    iter->ref_count--;
    if (iter->ref_count == 0) {
        // Clean up iterator-specific data
        if (iter->type == ITER_ARRAY) {
            da_release(&iter->data.array_iter.array);
        } else if (iter->type == ITER_RANGE) {
            vm_release(iter->data.range_iter.current);
            vm_release(iter->data.range_iter.end);
            vm_release(iter->data.range_iter.step);
        }

        // Free the iterator itself
        free(iter);
    }
}