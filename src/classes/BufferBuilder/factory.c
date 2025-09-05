#include "buffer_builder.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include <stdint.h>

// BufferBuilder(capacity) - Constructor function
value_t buffer_builder_factory(vm_t* vm, class_t* self, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferBuilder() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t capacity_val = args[0];
    if (capacity_val.type != VAL_INT32) {
        runtime_error(vm, "BufferBuilder() requires an integer capacity, not %s", value_type_name(capacity_val.type));
    }

    int32_t capacity = capacity_val.as.int32;
    if (capacity < 0) {
        runtime_error(vm, "BufferBuilder() capacity must be non-negative");
    }

    // Create reference counted builder directly
    db_builder builder = db_builder_new((size_t)capacity);
    return make_buffer_builder(builder);
}