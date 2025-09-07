#include "array.h"
#include "builtins.h"
#include "dynamic_array.h"
#include "vm.h"

static int is_callable(value_t v) {
    return v.type == VAL_NATIVE || v.type == VAL_CLOSURE ||
           v.type == VAL_FUNCTION || v.type == VAL_BOUND_METHOD;
}

value_t builtin_array_map(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "map() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t mapper   = args[1];

    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "map() can only be called on arrays");
    }
    if (!is_callable(mapper)) {
        runtime_error(vm, "map() expects a function");
    }

    da_array in  = receiver.as.array;
    size_t   len = da_length(in);

    da_array out = da_new(sizeof(value_t));
    da_reserve(out, len);

    for (size_t i = 0; i < len; i++) {
        value_t* elem = (value_t*)da_get(in, i);

        // Build (element, index, array)
        value_t call_args[3];
        call_args[0] = vm_retain(*elem);               // element
        call_args[1] = make_int32((int32_t)i);         // index
        call_args[2] = vm_retain(receiver);            // array

        value_t mapped = vm_call_slate_function_safe(vm, mapper, 3, call_args);

        // Release our retained args (ownership of return is ours)
        vm_release(call_args[0]);
        vm_release(call_args[2]);

        // Push the result (copy the value_t; retain if it holds refs)
        value_t stored = vm_retain(mapped);
        da_push(out, &stored);

        // We own 'mapped'; releasing our temporary is safe after retain
        vm_release(mapped);
    }

    return make_array(out);
}

value_t builtin_array_filter(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "filter() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t predicate = args[1];

    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "filter() can only be called on arrays");
    }
    if (!is_callable(predicate)) {
        runtime_error(vm, "filter() expects a function");
    }

    da_array in = receiver.as.array;
    size_t len = da_length(in);

    da_array out = da_new(sizeof(value_t));

    for (size_t i = 0; i < len; i++) {
        value_t* elem = (value_t*)da_get(in, i);

        // Build (element, index, array)
        value_t call_args[3];
        call_args[0] = vm_retain(*elem);               // element
        call_args[1] = make_int32((int32_t)i);         // index
        call_args[2] = vm_retain(receiver);            // array

        value_t result = vm_call_slate_function_safe(vm, predicate, 3, call_args);

        // Release our retained args
        vm_release(call_args[0]);
        vm_release(call_args[2]);

        // Check if result is truthy
        if (is_truthy(result)) {
            // Retain the element and add to result
            value_t retained = vm_retain(*elem);
            da_push(out, &retained);
        }

        // Release the result from the function call
        vm_release(result);
    }

    return make_array(out);
}

value_t builtin_array_flatmap(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 2) {
        runtime_error(vm, "flatMap() takes exactly 1 argument (%d given)", arg_count - 1);
    }

    value_t receiver = args[0];
    value_t mapper   = args[1];

    if (receiver.type != VAL_ARRAY) {
        runtime_error(vm, "flatMap() can only be called on arrays");
    }
    if (!is_callable(mapper)) {
        runtime_error(vm, "flatMap() expects a function");
    }

    da_array in      = receiver.as.array;
    size_t   len     = da_length(in);
    da_array result  = da_new(sizeof(value_t));

    for (size_t i = 0; i < len; i++) {
        value_t* elem = (value_t*)da_get(in, i);

        value_t call_args[3];
        call_args[0] = vm_retain(*elem);
        call_args[1] = make_int32((int32_t)i);
        call_args[2] = vm_retain(receiver);

        value_t mapped = vm_call_slate_function_safe(vm, mapper, 3, call_args);

        vm_release(call_args[0]);
        vm_release(call_args[2]);

        if (mapped.type == VAL_ARRAY && mapped.as.array) {
            da_array mapped_arr = mapped.as.array;
            size_t    mlen      = da_length(mapped_arr);
            for (size_t j = 0; j < mlen; j++) {
                value_t* me = (value_t*)da_get(mapped_arr, j);
                value_t   c = vm_retain(*me);
                da_push(result, &c);
            }
        } else {
            value_t c = vm_retain(mapped);
            da_push(result, &c);
        }

        vm_release(mapped);
    }

    return make_array(result);
}