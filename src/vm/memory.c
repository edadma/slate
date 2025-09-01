#include "vm.h"
#include <stdlib.h>

// Bound method reference counting functions
bound_method_t* bound_method_retain(bound_method_t* method) {
    if (!method)
        return method;
    method->ref_count++;
    return method;
}

void bound_method_release(bound_method_t* method) {
    if (!method)
        return;

    method->ref_count--;
    if (method->ref_count == 0) {
        // Clean up bound method data
        vm_release(method->receiver);

        // Free the bound method itself
        free(method);
    }
}

// Class reference counting functions

// Range reference counting functions
range_t* range_retain(range_t* range) {
    if (!range)
        return range;
    range->ref_count++;
    return range;
}

void range_release(range_t* range) {
    if (!range)
        return;

    range->ref_count--;
    if (range->ref_count == 0) {
        // Clean up range data
        vm_release(range->start);
        vm_release(range->end);

        // Free the range itself
        free(range);
    }
}