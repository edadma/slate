#include "class_boolean.h"
#include "builtins.h"

// Boolean factory function - converts any value to boolean based on truthiness
value_t boolean_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    // Case 1: No arguments - return false
    if (arg_count == 0) {
        return make_boolean(false);
    }
    
    // Case 2: One argument - convert to boolean based on truthiness
    if (arg_count == 1) {
        return make_boolean(is_truthy(args[0]));
    }
    
    // Case 3: More than one argument - error
    runtime_error(vm, "Boolean() takes 0 or 1 arguments (%d given)", arg_count);
    return make_null(); // Won't reach here due to exit in runtime_error
}