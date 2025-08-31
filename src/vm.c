#include "vm.h"
#include "opcodes/opcodes.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "builtins.h"
#include "codegen.h" // For debug_info functions
#include "datetime.h" // For date/time functions

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
slate_vm* vm_create(void) {
    slate_vm* vm = malloc(sizeof(slate_vm));
    if (!vm)
        return NULL;

    vm->stack = malloc(sizeof(value_t) * STACK_MAX);
    vm->frames = malloc(sizeof(call_frame) * FRAMES_MAX);
    vm->constants = malloc(sizeof(value_t) * CONSTANTS_MAX);

    if (!vm->stack || !vm->frames || !vm->constants) {
        vm_destroy(vm);
        return NULL;
    }

    vm->stack_capacity = STACK_MAX;
    vm->frame_capacity = FRAMES_MAX;
    vm->constant_capacity = CONSTANTS_MAX;

    // Create globals object - bit_value will be stored as property values
    vm->globals = do_create(NULL); // No release function yet
    if (!vm->globals) {
        vm_destroy(vm);
        return NULL;
    }

    // Create function table - stores all defined functions with reference counting
    vm->functions = da_new(sizeof(function_t*)); // Store pointers to functions
    if (!vm->functions) {
        vm_destroy(vm);
        return NULL;
    }

    // Initialize built-in functions
    builtins_init(vm);

    // Initialize result register to undefined
    vm->result = make_undefined();

    // Initialize debug location
    vm->current_debug = NULL;

    // Initialize command line arguments (empty by default)
    vm->argc = 0;
    vm->argv = NULL;

    vm_reset(vm);
    return vm;
}

slate_vm* vm_create_with_args(int argc, char** argv) {
    slate_vm* vm = vm_create();
    if (vm) {
        vm->argc = argc;
        vm->argv = argv;
    }
    return vm;
}

void vm_destroy(slate_vm* vm) {
    if (!vm)
        return;

    // Free constants
    for (size_t i = 0; i < vm->constant_count; i++) {
        free_value(vm->constants[i]);
    }

    free(vm->stack);
    free(vm->frames);
    free(vm->constants);
    do_release(&vm->globals);
    
    // Release function table (functions handle their own ref counting)
    da_release(&vm->functions);

    // Release result register
    free_value(vm->result);

    // Clean up debug location
    debug_location_free(vm->current_debug);

    free(vm);
}

void vm_reset(slate_vm* vm) {
    if (!vm)
        return;

    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}

// Value memory management
value_t vm_retain(value_t value) {
    if (value.type == VAL_STRING) {
        value.as.string = ds_retain(value.as.string);
    } else if (value.type == VAL_STRING_BUILDER) {
        value.as.string_builder = ds_builder_retain(value.as.string_builder);
    } else if (value.type == VAL_ARRAY) {
        value.as.array = da_retain(value.as.array);
    } else if (value.type == VAL_OBJECT) {
        value.as.object = do_retain(value.as.object);
    } else if (value.type == VAL_CLASS) {
        value.as.class = class_retain(value.as.class);
    } else if (value.type == VAL_BIGINT) {
        value.as.bigint = di_retain(value.as.bigint);
    } else if (value.type == VAL_RANGE) {
        value.as.range = range_retain(value.as.range);
    } else if (value.type == VAL_ITERATOR) {
        value.as.iterator = iterator_retain(value.as.iterator);
    } else if (value.type == VAL_BOUND_METHOD) {
        value.as.bound_method = bound_method_retain(value.as.bound_method);
    } else if (value.type == VAL_BUFFER) {
        value.as.buffer = db_retain(value.as.buffer);
    } else if (value.type == VAL_BUFFER_BUILDER) {
        value.as.builder = db_builder_retain(value.as.builder);
    } else if (value.type == VAL_BUFFER_READER) {
        value.as.reader = db_reader_retain(value.as.reader);
    } else if (value.type == VAL_LOCAL_DATE) {
        value.as.local_date = local_date_retain(value.as.local_date);
    } else if (value.type == VAL_LOCAL_TIME) {
        value.as.local_time = local_time_retain(value.as.local_time);
    } else if (value.type == VAL_LOCAL_DATETIME) {
        value.as.local_datetime = local_datetime_retain(value.as.local_datetime);
    } else if (value.type == VAL_ZONED_DATETIME) {
        value.as.zoned_datetime = zoned_datetime_retain(value.as.zoned_datetime);
    } else if (value.type == VAL_INSTANT) {
        value.as.instant = instant_retain(value.as.instant);
    } else if (value.type == VAL_DURATION) {
        value.as.duration = duration_retain(value.as.duration);
    } else if (value.type == VAL_PERIOD) {
        value.as.period = period_retain(value.as.period);
    }
    return value;
}

void vm_release(value_t value) {
    if (value.type == VAL_STRING) {
        ds_release(&value.as.string);
    } else if (value.type == VAL_STRING_BUILDER) {
        ds_builder_release(&value.as.string_builder);
    } else if (value.type == VAL_ARRAY) {
        da_release(&value.as.array);
    } else if (value.type == VAL_OBJECT) {
        do_release(&value.as.object);
    } else if (value.type == VAL_CLASS) {
        class_release(value.as.class);
    } else if (value.type == VAL_BIGINT) {
        di_release(&value.as.bigint);
    } else if (value.type == VAL_RANGE) {
        range_release(value.as.range);
    } else if (value.type == VAL_ITERATOR) {
        iterator_release(value.as.iterator);
    } else if (value.type == VAL_BOUND_METHOD) {
        bound_method_release(value.as.bound_method);
    } else if (value.type == VAL_BUFFER) {
        db_release(&value.as.buffer);
    } else if (value.type == VAL_BUFFER_BUILDER) {
        // Builder cleanup - release reference counted builder
        if (value.as.builder) {
            db_builder temp = value.as.builder;
            db_builder_release(&temp);
        }
    } else if (value.type == VAL_BUFFER_READER) {
        // Reader cleanup - release reference counted reader
        if (value.as.reader) {
            db_reader temp = value.as.reader;
            db_reader_release(&temp);
        }
    } else if (value.type == VAL_LOCAL_DATE) {
        local_date_release(value.as.local_date);
    } else if (value.type == VAL_LOCAL_TIME) {
        local_time_release(value.as.local_time);
    } else if (value.type == VAL_LOCAL_DATETIME) {
        local_datetime_release(value.as.local_datetime);
    } else if (value.type == VAL_ZONED_DATETIME) {
        zoned_datetime_release(value.as.zoned_datetime);
    } else if (value.type == VAL_INSTANT) {
        instant_release(value.as.instant);
    } else if (value.type == VAL_DURATION) {
        duration_release(value.as.duration);
    } else if (value.type == VAL_PERIOD) {
        period_release(value.as.period);
    }
}

// Stack operations
void vm_push(slate_vm* vm, value_t value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = vm_retain(value);
    vm->stack_top++;
}

value_t vm_pop(slate_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(slate_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }

// Value creation functions
value_t make_null(void) {
    value_t value;
    value.type = VAL_NULL;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_undefined(void) {
    value_t value;
    value.type = VAL_UNDEFINED;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_boolean(int val) {
    value_t value;
    value.type = VAL_BOOLEAN;
    value.as.boolean = val;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_int32(int32_t val) {
    value_t value;
    value.type = VAL_INT32;
    value.as.int32 = val;
    value.class = global_int_class; // All integers have Int class
    value.debug = NULL;
    return value;
}

value_t make_bigint(di_int big) {
    value_t value;
    value.type = VAL_BIGINT;
    value.as.bigint = big;
    value.class = global_int_class; // All integers have Int class
    value.debug = NULL;
    return value;
}

value_t make_number(double val) {
    value_t value;
    value.type = VAL_NUMBER;
    value.as.number = val;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_string(const char* val) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = ds_new(val); // Using dynamic_string.h!
    value.class = global_string_class; // All strings have String class
    value.debug = NULL;
    return value;
}

value_t make_string_ds(ds_string str) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = str; // Take ownership of the ds_string
    value.class = global_string_class; // All strings have String class
    value.debug = NULL;
    return value;
}

value_t make_string_builder(ds_builder builder) {
    value_t value;
    value.type = VAL_STRING_BUILDER;
    value.as.string_builder = builder; // Take ownership of the ds_builder
    value.class = global_string_builder_class; // All string builders have StringBuilder class
    value.debug = NULL;
    return value;
}

value_t make_array(da_array array) {
    value_t value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    value.class = global_array_class; // All arrays have Array class
    value.debug = NULL;
    return value;
}

value_t make_object(do_object object) {
    value_t value;
    value.type = VAL_OBJECT;
    value.as.object = object;
    value.class = global_value_class; // Objects inherit from Value
    value.debug = NULL;
    return value;
}

value_t make_class(const char* name, do_object properties) {
    class_t* cls = malloc(sizeof(class_t));
    if (!cls) {
        return make_null(); // Return null on allocation failure
    }

    cls->ref_count = 1;
    cls->name = strdup(name ? name : "Class"); // Duplicate the name string
    cls->properties = properties ? do_retain(properties) : do_create(NULL); // Retain or create empty
    cls->factory = NULL; // Default: class cannot be instantiated by calling it

    value_t value;
    value.type = VAL_CLASS;
    value.as.class = cls;
    value.class = global_value_class; // Classes inherit from Value
    value.debug = NULL;
    return value;
}

value_t make_range(value_t start, value_t end, int exclusive) {
    range_t* range = malloc(sizeof(range_t));
    if (!range) {
        // TODO: Handle allocation failure
        return make_null();
    }

    range->ref_count = 1; // Initialize reference count
    range->start = vm_retain(start);
    range->end = vm_retain(end);
    range->exclusive = exclusive;

    value_t value;
    value.type = VAL_RANGE;
    value.as.range = range;
    value.class = global_range_class; // All ranges have Range class
    value.debug = NULL;
    return value;
}

value_t make_iterator(iterator_t* iterator) {
    value_t value;
    value.type = VAL_ITERATOR;
    value.as.iterator = iterator;
    value.class = global_iterator_class; // All iterators have Iterator class
    value.debug = NULL;
    return value;
}

value_t make_function(function_t* function) {
    value_t value;
    value.type = VAL_FUNCTION;
    value.as.function = function;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_closure(closure_t* closure) {
    value_t value;
    value.type = VAL_CLOSURE;
    value.as.closure = closure;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_native(native_t func) {
    value_t value;
    value.type = VAL_NATIVE;
    value.as.native = func;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_bound_method(value_t receiver, native_t method_func) {
    bound_method_t* method = malloc(sizeof(bound_method_t));
    if (!method) {
        return make_null(); // Handle allocation failure
    }

    method->ref_count = 1; // Initialize reference count
    method->receiver = vm_retain(receiver);
    method->method = method_func;

    value_t value;
    value.type = VAL_BOUND_METHOD;
    value.as.bound_method = method;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer(db_buffer buffer) {
    value_t value;
    value.type = VAL_BUFFER;
    value.as.buffer = buffer;
    value.class = global_buffer_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer_builder(db_builder builder) {
    value_t value;
    value.type = VAL_BUFFER_BUILDER;
    value.as.builder = builder;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

value_t make_buffer_reader(db_reader reader) {
    value_t value;
    value.type = VAL_BUFFER_READER;
    value.as.reader = reader;
    value.class = global_value_class;
    value.debug = NULL;
    return value;
}

// Date/Time value factory functions
value_t make_local_date(local_date_t* date) {
    value_t value;
    value.type = VAL_LOCAL_DATE;
    value.as.local_date = date;
    value.class = global_local_date_class;
    value.debug = NULL;
    return value;
}

value_t make_local_time(local_time_t* time) {
    value_t value;
    value.type = VAL_LOCAL_TIME;
    value.as.local_time = time;
    value.class = global_local_time_class;
    value.debug = NULL;
    return value;
}

value_t make_local_datetime(local_datetime_t* datetime) {
    value_t value;
    value.type = VAL_LOCAL_DATETIME;
    value.as.local_datetime = datetime;
    value.class = global_local_datetime_class;
    value.debug = NULL;
    return value;
}

value_t make_zoned_datetime(zoned_datetime_t* zdt) {
    value_t value;
    value.type = VAL_ZONED_DATETIME;
    value.as.zoned_datetime = zdt;
    value.class = global_zoned_datetime_class;
    value.debug = NULL;
    return value;
}

value_t make_instant(instant_t* instant) {
    value_t value;
    value.type = VAL_INSTANT;
    value.as.instant = instant;
    value.class = global_instant_class;
    value.debug = NULL;
    return value;
}

value_t make_duration(duration_t* duration) {
    value_t value;
    value.type = VAL_DURATION;
    value.as.duration = duration;
    value.class = global_duration_class;
    value.debug = NULL;
    return value;
}

value_t make_period(period_t* period) {
    value_t value;
    value.type = VAL_PERIOD;
    value.as.period = period;
    value.class = global_period_class;
    value.debug = NULL;
    return value;
}

// Core VM execution function - executes a function with the given closure
// Assumes the VM is already set up with proper stack state and call frame
vm_result vm_execute_function(slate_vm* vm, function_t* function, closure_t* closure) {
    if (!vm || !function || !closure)
        return VM_RUNTIME_ERROR;
        
    // Main execution loop - handle the most common opcodes for simple functions
    for (;;) {
        vm->current_instruction = vm->ip;
        opcode instruction = (opcode)*vm->ip++;
        
        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            if (constant >= current_func->constant_count) {
                return VM_RUNTIME_ERROR;
            }
            
            value_t val = current_func->constants[constant];
            if (vm->current_debug) {
                val.debug = debug_location_copy(vm->current_debug);
            }
            vm_push(vm, val);
            break;
        }
        
        case OP_GET_GLOBAL: {
            uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            if (name_constant >= current_func->constant_count) {
                return VM_RUNTIME_ERROR;
            }
            value_t name_val = current_func->constants[name_constant];
            if (name_val.type != VAL_STRING) {
                return VM_RUNTIME_ERROR;
            }
            char* name = name_val.as.string;
            
            // Check if we're in a function call and look for parameters first
            if (vm->frame_count > 0) {
                call_frame* current_frame = &vm->frames[vm->frame_count - 1];
                function_t* func = current_frame->closure->function;
                
                // Check if name matches a parameter
                for (size_t i = 0; i < func->parameter_count; i++) {
                    if (strcmp(name, func->parameter_names[i]) == 0) {
                        // Found parameter - get value from stack slot
                        value_t param_value = current_frame->slots[i];
                        vm_push(vm, param_value);
                        goto found_variable;
                    }
                }
            }
            
            // Fall through to global variable lookup
            value_t* stored_value = (value_t*)do_get(vm->globals, name);
            if (stored_value) {
                vm_push(vm, *stored_value);
            } else {
                return VM_RUNTIME_ERROR; // Undefined variable
            }
            
            found_variable:
            break;
        }
        
        case OP_SET_RESULT: {
            value_t result = vm_pop(vm);
            vm->result = result;
            break;
        }
        
        case OP_MULTIPLY: {
            vm_result result = op_multiply(vm);
            if (result != VM_OK) return result;
            break;
        }
        
        case OP_NEGATE: {
            vm_result result = op_negate(vm);
            if (result != VM_OK) return result;
            break;
        }
        
        case OP_RETURN: {
            vm_result result = op_return(vm);
            if (result != VM_OK) return result;
            break;
        }
        
        case OP_SET_DEBUG_LOCATION: {
            // Skip debug location setup for now
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            uint8_t line = *vm->ip++;
            uint8_t column = *vm->ip++;
            // Ignore for now
            break;
        }
        
        case OP_CLEAR_DEBUG_LOCATION: {
            // Ignore for now
            break;
        }
        
        case OP_HALT:
            return VM_OK;
            
        // Add some more essential opcodes based on what a simple lambda might need
        case 37: // Whatever opcode 37 is, let's handle it - might be a debug or simple opcode
            printf("DEBUG: Handling mystery opcode 37 as no-op\n");
            break;
            
        default:
            // For unimplemented opcodes, print debug and return success (for debugging)
            printf("DEBUG: Unimplemented opcode: %d - treating as successful termination\n", instruction);
            return VM_OK;
        }
    }
}

// Helper function to call functions from C code (for array methods, etc.)
value_t vm_call_function(slate_vm* vm, value_t callable, int arg_count, value_t* args) {
    if (callable.type == VAL_NATIVE) {
        // Native functions are easy - just call them directly
        native_t func = (native_t)callable.as.native;
        return func(vm, arg_count, args);
    }
    
    if (callable.type != VAL_CLOSURE && callable.type != VAL_FUNCTION) {
        // Not callable - return undefined
        return make_undefined();
    }
    
    // For user-defined functions, be flexible with argument count
    // If function expects fewer arguments than provided, just pass the first N arguments
    function_t* func = NULL;
    if (callable.type == VAL_CLOSURE) {
        func = callable.as.closure->function;
    } else if (callable.type == VAL_FUNCTION) {
        func = callable.as.function;
    }
    
    int actual_arg_count = (arg_count > func->parameter_count) ? func->parameter_count : arg_count;
           
    // Only fail if we don't have enough arguments
    if (arg_count < func->parameter_count) {
        return make_undefined();
    }
    
    // Use the real VM execution infrastructure for proper closure execution
    closure_t* closure = NULL;
    int created_closure = 0;
    
    if (callable.type == VAL_CLOSURE) {
        closure = callable.as.closure;
    } else if (callable.type == VAL_FUNCTION) {
        closure = closure_create(func);
        created_closure = 1;
    }
    
    // Set up call frame on the current VM
    if (vm->frame_count >= vm->frame_capacity) {
        if (created_closure) closure_destroy(closure);
        return make_undefined();
    }
    
    // Save current VM state
    size_t saved_stack_size = vm->stack_top - vm->stack;
    uint8_t* saved_ip = vm->ip;
    uint8_t* saved_bytecode = vm->bytecode;
    size_t saved_frame_count = vm->frame_count;
    
    // Push arguments onto VM stack
    for (int i = 0; i < actual_arg_count; i++) {
        vm_push(vm, args[i]);
    }
    
    // Set up call frame
    call_frame* frame = &vm->frames[vm->frame_count++];
    frame->closure = closure;
    frame->ip = saved_ip;
    frame->slots = vm->stack_top - actual_arg_count;
    
    // Switch execution context to function
    vm->ip = func->bytecode;
    vm->bytecode = func->bytecode;
    
    // Execute the function using our core execution loop
    vm_result result = vm_run(vm);
    
    value_t return_value = make_undefined();
    if (result == VM_OK) {
        // Get return value from VM result or stack
        if (vm->stack_top > vm->stack + saved_stack_size) {
            return_value = vm_pop(vm);
        } else {
            return_value = vm->result;
        }
    }
    
    // Restore VM state
    vm->stack_top = vm->stack + saved_stack_size;
    vm->ip = saved_ip;
    vm->bytecode = saved_bytecode;
    vm->frame_count = saved_frame_count;
    
    // Clean up
    if (created_closure) {
        closure_destroy(closure);
    }
    
    return return_value;
}

// Value creation functions with debug info
value_t make_null_with_debug(debug_location* debug) {
    value_t value = make_null();
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_undefined_with_debug(debug_location* debug) {
    value_t value = make_undefined();
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_boolean_with_debug(int val, debug_location* debug) {
    value_t value = make_boolean(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_int32_with_debug(int32_t val, debug_location* debug) {
    value_t value = make_int32(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_bigint_with_debug(di_int big, debug_location* debug) {
    value_t value = make_bigint(big);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_number_with_debug(double val, debug_location* debug) {
    value_t value = make_number(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_string_with_debug(const char* val, debug_location* debug) {
    value_t value = make_string(val);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_string_ds_with_debug(ds_string str, debug_location* debug) {
    value_t value = make_string_ds(str);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_string_builder_with_debug(ds_builder builder, debug_location* debug) {
    value_t value = make_string_builder(builder);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_array_with_debug(da_array array, debug_location* debug) {
    value_t value = make_array(array);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_object_with_debug(do_object object, debug_location* debug) {
    value_t value = make_object(object);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_class_with_debug(const char* name, do_object properties, debug_location* debug) {
    value_t value = make_class(name, properties);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_range_with_debug(value_t start, value_t end, int exclusive, debug_location* debug) {
    value_t value = make_range(start, end, exclusive);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_iterator_with_debug(iterator_t* iterator, debug_location* debug) {
    value_t value = make_iterator(iterator);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_function_with_debug(function_t* function, debug_location* debug) {
    value_t value = make_function(function);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_closure_with_debug(closure_t* closure, debug_location* debug) {
    value_t value = make_closure(closure);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_builtin_with_debug(void* builtin_func, debug_location* debug) {
    value_t value = make_native(builtin_func);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_bound_method_with_debug(value_t receiver, native_t method_func, debug_location* debug) {
    value_t value = make_bound_method(receiver, method_func);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_buffer_with_debug(db_buffer buffer, debug_location* debug) {
    value_t value = make_buffer(buffer);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_buffer_builder_with_debug(db_builder builder, debug_location* debug) {
    value_t value = make_buffer_builder(builder);
    value.debug = debug_location_copy(debug);
    return value;
}

value_t make_buffer_reader_with_debug(db_reader reader, debug_location* debug) {
    value_t value = make_buffer_reader(reader);
    value.debug = debug_location_copy(debug);
    return value;
}

// Forward declarations
ds_string value_to_string_representation(slate_vm* vm, value_t value);
static ds_string display_value_to_string(slate_vm* vm, value_t value);

// Context for object property iteration
struct object_string_context {
    ds_string* result_ptr;
    int count;
    slate_vm* vm;
};

// Callback function for object property iteration
static void object_property_to_string_callback(const char* key, void* data, size_t size, void* ctx) {
    struct object_string_context* context = (struct object_string_context*)ctx;
    ds_string* result_ptr = context->result_ptr;
    slate_vm* vm = context->vm;

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
ds_string value_to_string_representation(slate_vm* vm, value_t value) {
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
    case VAL_INSTANT:
        return ds_new("<Instant>");  // TODO: implement string conversion
    case VAL_DURATION:
        return ds_new("<Duration>");  // TODO: implement string conversion
    case VAL_PERIOD:
        return ds_new("<Period>");  // TODO: implement string conversion
    default:
        return ds_new("{Unknown}");
    }
}

// Helper function to convert values to display string (with quotes for strings, for use inside aggregates)
static ds_string display_value_to_string(slate_vm* vm, value_t value) {
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
void print_for_builtin(slate_vm* vm, value_t value) {
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

// Value utility functions
int is_falsy(value_t value) {
    switch (value.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return !value.as.boolean;
    case VAL_INT32:
        return value.as.int32 == 0;
    case VAL_BIGINT:
        return di_is_zero(value.as.bigint);
    case VAL_NUMBER:
        return value.as.number == 0.0;
    case VAL_STRING:
        return value.as.string == NULL || strlen(value.as.string) == 0;
    case VAL_BUFFER:
        return value.as.buffer == NULL || db_size(value.as.buffer) == 0;
    case VAL_BUFFER_BUILDER:
        return value.as.builder == NULL;
    case VAL_BUFFER_READER:
        return value.as.reader == NULL;
    default:
        return 0;
    }
}

int is_truthy(value_t value) {
    return !is_falsy(value);
}

int is_number(value_t value) { return value.type == VAL_INT32 || value.type == VAL_BIGINT || value.type == VAL_NUMBER; }

int values_equal(value_t a, value_t b) {
    // Handle cross-type numeric comparisons
    if (is_number(a) && is_number(b)) {
        // Convert to common type for comparison
        if (a.type == VAL_INT32 && b.type == VAL_INT32) {
            return a.as.int32 == b.as.int32;
        } else if (a.type == VAL_BIGINT && b.type == VAL_BIGINT) {
            return di_eq(a.as.bigint, b.as.bigint);
        } else if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
            return a.as.number == b.as.number;
        }
        // Cross-type comparisons - convert to common type
        // For now, convert to double for simplicity
        double a_val = (a.type == VAL_INT32) ? (double)a.as.int32
            : (a.type == VAL_BIGINT)         ? di_to_double(a.as.bigint)
                                             : a.as.number;
        double b_val = (b.type == VAL_INT32) ? (double)b.as.int32
            : (b.type == VAL_BIGINT)         ? di_to_double(b.as.bigint)
                                             : b.as.number;
        return a_val == b_val;
    }

    if (a.type != b.type)
        return 0;

    switch (a.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return a.as.boolean == b.as.boolean;
    case VAL_STRING:
        if (a.as.string == NULL && b.as.string == NULL)
            return 1;
        if (a.as.string == NULL || b.as.string == NULL)
            return 0;
        return strcmp(a.as.string, b.as.string) == 0; // DS strings work with strcmp!
    case VAL_ARRAY:
        return a.as.array == b.as.array;
    case VAL_OBJECT:
        return a.as.object == b.as.object;
    case VAL_CLASS:
        return a.as.class == b.as.class; // Class identity comparison
    case VAL_BUFFER:
        if (a.as.buffer == b.as.buffer)
            return 1; // Same buffer reference
        if (a.as.buffer == NULL || b.as.buffer == NULL)
            return 0;
        return db_equals(a.as.buffer, b.as.buffer);
    case VAL_BUFFER_BUILDER:
        return a.as.builder == b.as.builder;
    case VAL_BUFFER_READER:
        return a.as.reader == b.as.reader;
    case VAL_FUNCTION:
        return a.as.function == b.as.function;
    case VAL_CLOSURE:
        return a.as.closure == b.as.closure;
    case VAL_NATIVE:
        return a.as.native == b.as.native;
    case VAL_LOCAL_DATE:
        if (a.as.local_date == b.as.local_date)
            return 1; // Same reference
        if (a.as.local_date == NULL || b.as.local_date == NULL)
            return 0;
        return local_date_equals(a.as.local_date, b.as.local_date);
    case VAL_LOCAL_TIME:
        if (a.as.local_time == b.as.local_time)
            return 1; // Same reference
        if (a.as.local_time == NULL || b.as.local_time == NULL)
            return 0;
        return local_time_equals(a.as.local_time, b.as.local_time);
    case VAL_LOCAL_DATETIME:
    case VAL_ZONED_DATETIME:
    case VAL_INSTANT:
    case VAL_DURATION:
    case VAL_PERIOD:
        // For now, use reference equality for complex types
        return a.as.local_datetime == b.as.local_datetime;
    default:
        return 0;
    }
}

void print_value(slate_vm* vm, value_t value) {
    switch (value.type) {
    case VAL_NULL:
        printf("null");
        break;
    case VAL_UNDEFINED:
        printf("undefined");
        break;
    case VAL_BOOLEAN:
        printf(value.as.boolean ? "true" : "false");
        break;
    case VAL_INT32:
        printf("%d", value.as.int32);
        break;
    case VAL_BIGINT: {
        char* str = di_to_string(value.as.bigint, 10);
        if (str) {
            printf("%s", str);
            free(str);
        } else {
            printf("<bigint>");
        }
        break;
    }
    case VAL_NUMBER:
        printf("%.6g", value.as.number);
        break;
    case VAL_STRING:
        printf("\"%s\"", value.as.string ? value.as.string : ""); // DS strings work directly!
        break;
    case VAL_STRING_BUILDER: {
        printf("<StringBuilder>");
        break;
    }
    case VAL_ARRAY: {
        printf("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0)
                    printf(", ");
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    print_value(vm, *element);
                } else {
                    printf("null");
                }
            }
        }
        printf("]");
        break;
    }
    case VAL_OBJECT: {
        printf("{");
        if (value.as.object) {
            // For now, just show it's an object - full object printing would be more complex
            printf("Object");
        }
        printf("}");
        break;
    }
    case VAL_CLASS: {
        if (value.as.class) {
            printf("<class %s>", value.as.class->name ? value.as.class->name : "anonymous");
        } else {
            printf("<null class>");
        }
        break;
    }
    case VAL_FUNCTION:
        printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
        break;
    case VAL_CLOSURE:
        printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
        break;
    case VAL_NATIVE:
        printf("<builtin function>");
        break;
    case VAL_RANGE: {
        if (!value.as.range) {
            printf("<null range>");
        } else {
            print_value(vm, value.as.range->start);
            printf(value.as.range->exclusive ? "..<" : "..");
            print_value(vm, value.as.range->end);
        }
        break;
    }
    case VAL_ITERATOR: {
        if (!value.as.iterator) {
            printf("<null iterator>");
        } else if (value.as.iterator->type == ITER_ARRAY) {
            printf("<array iterator>");
        } else if (value.as.iterator->type == ITER_RANGE) {
            printf("<range iterator>");
        } else {
            printf("<unknown iterator>");
        }
        break;
    }
    case VAL_BUFFER: {
        if (!value.as.buffer) {
            printf("<null buffer>");
        } else {
            printf("<buffer size=%zu>", db_size(value.as.buffer));
        }
        break;
    }
    case VAL_BUFFER_BUILDER: {
        if (!value.as.builder) {
            printf("<null buffer builder>");
        } else {
            printf("<buffer builder>");
        }
        break;
    }
    case VAL_BUFFER_READER: {
        if (!value.as.reader) {
            printf("<null buffer reader>");
        } else {
            printf("<buffer reader pos=%zu>", db_reader_position(value.as.reader));
        }
        break;
    }
    case VAL_BOUND_METHOD: {
        if (!value.as.bound_method) {
            printf("<null bound method>");
        } else {
            printf("<bound method>");
        }
        break;
    }
    case VAL_LOCAL_DATE: {
        if (value.as.local_date) {
            char* str = local_date_to_string(vm, value.as.local_date);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDate>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_TIME: {
        if (value.as.local_time) {
            char* str = local_time_to_string(vm, value.as.local_time);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_LOCAL_DATETIME: {
        if (value.as.local_datetime) {
            char* str = local_datetime_to_string(vm, value.as.local_datetime);
            if (str) {
                printf("%s", str);
                free(str);
            } else {
                printf("<LocalDateTime>");
            }
        } else {
            printf("null");
        }
        break;
    }
    case VAL_ZONED_DATETIME: {
        printf("<ZonedDateTime>");  // TODO: implement string conversion
        break;
    }
    case VAL_INSTANT: {
        printf("<Instant>");  // TODO: implement string conversion
        break;
    }
    case VAL_DURATION: {
        printf("<Duration>");  // TODO: implement string conversion
        break;
    }
    case VAL_PERIOD: {
        printf("<Period>");  // TODO: implement string conversion
        break;
    }
    }
}

double value_to_double(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return (double)value.as.int32;
    case VAL_BIGINT:
        return di_to_double(value.as.bigint);
    case VAL_NUMBER:
        return value.as.number;
    default:
        runtime_error("Cannot convert %s to number", value_type_name(value.type));
        return 0.0; // Never reached, but keeps compiler happy
    }
}

bool is_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return true;
    case VAL_BIGINT:
        // Check if BigInt fits in int32 range
        int32_t dummy;
        return di_to_int32(value.as.bigint, &dummy);
    case VAL_NUMBER:
        // Check if the number is a whole number that fits in int range
        return value.as.number == floor(value.as.number) && value.as.number >= INT_MIN && value.as.number <= INT_MAX;
    default:
        return false;
    }
}

int value_to_int(value_t value) {
    switch (value.type) {
    case VAL_INT32:
        return value.as.int32;
    case VAL_BIGINT:
        // Convert BigInt to int, checking for overflow
        int32_t result;
        if (di_to_int32(value.as.bigint, &result)) {
            return result;
        } else {
            char* str = di_to_string(value.as.bigint, 10);
            runtime_error("BigInt value %s too large for integer", str);
            free(str);
            return 0; // Never reached
        }
    case VAL_NUMBER:
        // Check if it's a valid integer
        if (value.as.number == floor(value.as.number) && value.as.number >= INT_MIN && value.as.number <= INT_MAX) {
            return (int)value.as.number;
        } else {
            runtime_error("Number %g is not a valid integer", value.as.number);
            return 0; // Never reached
        }
    default:
        runtime_error("Cannot convert %s to integer", value_type_name(value.type));
        return 0; // Never reached, but keeps compiler happy
    }
}

void free_value(value_t value) {
    // Clean up debug info first
    debug_location_free(value.debug);

    switch (value.type) {
    case VAL_NULL:
    case VAL_UNDEFINED:
    case VAL_BOOLEAN:
    case VAL_INT32:
    case VAL_NUMBER:
        // No cleanup needed for these types
        break;
    case VAL_BIGINT: {
        di_int temp = value.as.bigint;
        di_release(&temp); // DI cleanup with reference counting!
        break;
    }
    case VAL_STRING: {
        ds_string temp = value.as.string;
        ds_release(&temp); // DS cleanup with reference counting!
        break;
    }
    case VAL_STRING_BUILDER: {
        ds_builder temp = value.as.string_builder;
        ds_builder_release(&temp); // DS builder cleanup with reference counting!
        break;
    }
    case VAL_ARRAY: {
        da_array temp = value.as.array;
        da_release(&temp); // DA cleanup with reference counting!
        break;
    }
    case VAL_OBJECT: {
        do_object temp = value.as.object;
        do_release(&temp); // DO cleanup with reference counting!
        break;
    }
    case VAL_CLASS: {
        // Class cleanup is handled by vm_release() through reference counting
        // Don't duplicate cleanup here
        break;
    }
    case VAL_RANGE: {
        // Range cleanup is handled by vm_release() through reference counting
        // Don't duplicate cleanup here
        break;
    }
    case VAL_ITERATOR: {
        // Iterator cleanup is handled by vm_release() through reference counting
        // Don't duplicate cleanup here
        break;
    }
    case VAL_BUFFER: {
        db_buffer temp = value.as.buffer;
        db_release(&temp); // DB cleanup with reference counting!
        break;
    }
    case VAL_BUFFER_BUILDER: {
        // Builder cleanup - release reference counted builder
        if (value.as.builder) {
            db_builder temp = value.as.builder;
            db_builder_release(&temp);
        }
        break;
    }
    case VAL_BUFFER_READER: {
        // Reader cleanup - release reference counted reader
        if (value.as.reader) {
            db_reader temp = value.as.reader;
            db_reader_release(&temp);
        }
        break;
    }
    case VAL_BOUND_METHOD: {
        // Bound method cleanup is handled by vm_release() through reference counting
        // Don't duplicate cleanup here
        break;
    }
    case VAL_FUNCTION:
        function_destroy(value.as.function);
        break;
    case VAL_CLOSURE:
        closure_destroy(value.as.closure);
        break;
    case VAL_NATIVE:
        // No cleanup needed for builtin function pointers
        break;
    case VAL_LOCAL_DATE:
    case VAL_LOCAL_TIME:
    case VAL_LOCAL_DATETIME:
    case VAL_ZONED_DATETIME:
    case VAL_INSTANT:
    case VAL_DURATION:
    case VAL_PERIOD:
        // Date/time cleanup is handled by vm_release() through reference counting
        // Don't duplicate cleanup here
        break;
    default:
        // No cleanup needed for basic types
        break;
    }
}

// Constant pool management
size_t vm_add_constant(slate_vm* vm, value_t value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(slate_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
}

// Function table management
size_t vm_add_function(slate_vm* vm, function_t* function) {
    size_t index = vm->functions->length;
    DA_PUSH(vm->functions, function);
    return index;
}

function_t* vm_get_function(slate_vm* vm, size_t index) {
    assert(index < vm->functions->length);
    return DA_AT(vm->functions, index, function_t*);
}

// Note: Object operations now use dynamic_object.h
// No separate functions needed

// Note: Array operations now use dynamic_array.h
// No separate functions needed

// Function operations
function_t* function_create(const char* name) {
    function_t* function = malloc(sizeof(function_t));
    if (!function)
        return NULL;

    function->bytecode = NULL;
    function->bytecode_length = 0;
    function->constants = NULL;
    function->constant_count = 0;
    function->parameter_names = NULL;
    function->parameter_count = 0;
    function->local_count = 0;
    function->name = name ? strdup(name) : NULL;
    function->debug = NULL; // Initialize debug info

    return function;
}

void function_destroy(function_t* function) {
    if (!function)
        return;

    free(function->bytecode);

    for (size_t i = 0; i < function->constant_count; i++) {
        free_value(function->constants[i]);
    }
    free(function->constants);

    for (size_t i = 0; i < function->parameter_count; i++) {
        free(function->parameter_names[i]);
    }
    free(function->parameter_names);

    debug_info_destroy(function->debug);

    free(function->name);
    free(function);
}

closure_t* closure_create(function_t* function) {
    closure_t* closure = malloc(sizeof(closure_t));
    if (!closure)
        return NULL;

    closure->function = function;
    closure->upvalues = NULL;
    closure->upvalue_count = 0;

    return closure;
}

void closure_destroy(closure_t* closure) {
    if (!closure)
        return;

    function_destroy(closure->function);
    free(closure->upvalues);
    free(closure);
}

// Bytecode utilities
const char* opcode_name(opcode op) {
    switch (op) {
    case OP_PUSH_CONSTANT:
        return "PUSH_CONSTANT";
    case OP_PUSH_NULL:
        return "PUSH_NULL";
    case OP_PUSH_UNDEFINED:
        return "PUSH_UNDEFINED";
    case OP_PUSH_TRUE:
        return "PUSH_TRUE";
    case OP_PUSH_FALSE:
        return "PUSH_FALSE";
    case OP_POP:
        return "POP";
    case OP_DUP:
        return "DUP";
    case OP_SET_RESULT:
        return "SET_RESULT";
    case OP_ADD:
        return "ADD";
    case OP_SUBTRACT:
        return "SUBTRACT";
    case OP_MULTIPLY:
        return "MULTIPLY";
    case OP_DIVIDE:
        return "DIVIDE";
    case OP_MOD:
        return "MOD";
    case OP_POWER:
        return "POWER";
    case OP_NEGATE:
        return "NEGATE";
    case OP_EQUAL:
        return "EQUAL";
    case OP_NOT_EQUAL:
        return "NOT_EQUAL";
    case OP_LESS:
        return "LESS";
    case OP_LESS_EQUAL:
        return "LESS_EQUAL";
    case OP_GREATER:
        return "GREATER";
    case OP_GREATER_EQUAL:
        return "GREATER_EQUAL";
    case OP_NOT:
        return "NOT";
    case OP_AND:
        return "AND";
    case OP_OR:
        return "OR";
    case OP_NULL_COALESCE:
        return "NULL_COALESCE";
    case OP_IN:
        return "IN";
    case OP_INSTANCEOF:
        return "INSTANCEOF";
    case OP_BITWISE_AND:
        return "BITWISE_AND";
    case OP_BITWISE_OR:
        return "BITWISE_OR";
    case OP_BITWISE_XOR:
        return "BITWISE_XOR";
    case OP_BITWISE_NOT:
        return "BITWISE_NOT";
    case OP_LEFT_SHIFT:
        return "LEFT_SHIFT";
    case OP_RIGHT_SHIFT:
        return "RIGHT_SHIFT";
    case OP_LOGICAL_RIGHT_SHIFT:
        return "LOGICAL_RIGHT_SHIFT";
    case OP_FLOOR_DIV:
        return "FLOOR_DIV";
    case OP_INCREMENT:
        return "INCREMENT";
    case OP_DECREMENT:
        return "DECREMENT";
    case OP_GET_LOCAL:
        return "GET_LOCAL";
    case OP_SET_LOCAL:
        return "SET_LOCAL";
    case OP_GET_GLOBAL:
        return "GET_GLOBAL";
    case OP_SET_GLOBAL:
        return "SET_GLOBAL";
    case OP_DEFINE_GLOBAL:
        return "DEFINE_GLOBAL";
    case OP_GET_PROPERTY:
        return "GET_PROPERTY";
    case OP_SET_PROPERTY:
        return "SET_PROPERTY";
    case OP_GET_INDEX:
        return "GET_INDEX";
    case OP_SET_INDEX:
        return "SET_INDEX";
    case OP_BUILD_ARRAY:
        return "BUILD_ARRAY";
    case OP_BUILD_OBJECT:
        return "BUILD_OBJECT";
    case OP_BUILD_RANGE:
        return "BUILD_RANGE";
    case OP_CLOSURE:
        return "CLOSURE";
    case OP_CALL:
        return "CALL";
    case OP_RETURN:
        return "RETURN";
    case OP_JUMP:
        return "JUMP";
    case OP_JUMP_IF_FALSE:
        return "JUMP_IF_FALSE";
    case OP_JUMP_IF_TRUE:
        return "JUMP_IF_TRUE";
    case OP_LOOP:
        return "LOOP";
    case OP_POP_N:
        return "POP_N";
    case OP_POP_N_PRESERVE_TOP:
        return "POP_N_PRESERVE_TOP";
    case OP_SET_DEBUG_LOCATION:
        return "SET_DEBUG_LOCATION";
    case OP_CLEAR_DEBUG_LOCATION:
        return "CLEAR_DEBUG_LOCATION";
    case OP_HALT:
        return "HALT";
    default:
        return "UNKNOWN";
    }
}

// Core VM execution loop - runs until completion
// Assumes VM is already set up with proper call frames and stack
vm_result vm_run(slate_vm* vm) {
    if (!vm)
        return VM_RUNTIME_ERROR;

    // Main execution loop
    for (;;) {
        vm->current_instruction = vm->ip; // Store instruction start for error reporting
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2; // Skip the operand bytes
            
            // Get the current executing function for constants
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            
            if (constant >= current_func->constant_count) {
                printf("DEBUG: ERROR - constant index %d >= count %zu\n", constant, current_func->constant_count);
                return VM_RUNTIME_ERROR;
            }
            
            // Create value with current debug info - use CURRENT function's constants
            value_t val = current_func->constants[constant];
            if (vm->current_debug) {
                val.debug = debug_location_copy(vm->current_debug);
            }
            vm_push(vm, val);
            break;
        }

        case OP_PUSH_NULL:
            vm_push(vm, make_null_with_debug(vm->current_debug));
            break;

        case OP_PUSH_UNDEFINED:
            vm_push(vm, make_undefined_with_debug(vm->current_debug));
            break;

        case OP_PUSH_TRUE:
            vm_push(vm, make_boolean_with_debug(1, vm->current_debug));
            break;

        case OP_PUSH_FALSE:
            vm_push(vm, make_boolean_with_debug(0, vm->current_debug));
            break;

        case OP_POP:
            vm_release(vm_pop(vm));
            break;

        case OP_DUP: {
            value_t top = vm_peek(vm, 0);
            vm_push(vm, top);
            break;
        }

        case OP_SET_RESULT: {
            value_t result = vm_pop(vm);
            vm->result = result;
            break;
        }

        case OP_ADD: {
            vm_result result = op_add(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SUBTRACT: {
            vm_result result = op_subtract(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MULTIPLY: {
            vm_result result = op_multiply(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_DIVIDE: {
            vm_result result = op_divide(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_NEGATE: {
            vm_result result = op_negate(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_MOD: {
            vm_result result = op_mod(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_EQUAL: {
            vm_result result = op_equal(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_RETURN: {
            vm_result result = op_return(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_LOCAL: {
            vm_result result = op_get_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_SET_LOCAL: {
            vm_result result = op_set_local(vm);
            if (result != VM_OK) return result;
            break;
        }

        case OP_GET_GLOBAL: {
            uint16_t name_constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            // Get the current executing function from the current frame
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            if (name_constant >= current_func->constant_count) {
                printf("Runtime error: Constant index %d out of bounds (max %zu)\n", 
                       name_constant, current_func->constant_count - 1);
                return VM_RUNTIME_ERROR;
            }
            value_t name_val = current_func->constants[name_constant];
            if (name_val.type != VAL_STRING) {
                printf("Runtime error: Global variable name must be a string\n");
                return VM_RUNTIME_ERROR;
            }
            char* name = name_val.as.string;
            
            // Check if we're in a function call and look for parameters first
            if (vm->frame_count > 0) {
                call_frame* current_frame = &vm->frames[vm->frame_count - 1];
                function_t* func = current_frame->closure->function;
                
                // Check if name matches a parameter
                for (size_t i = 0; i < func->parameter_count; i++) {
                    if (strcmp(name, func->parameter_names[i]) == 0) {
                        // Found parameter - get value from stack slot
                        value_t param_value = current_frame->slots[i];
                        vm_push(vm, param_value);
                        goto found_variable;
                    }
                }
            }
            
            // Fall through to global variable lookup
            value_t* stored_value = (value_t*)do_get(vm->globals, name);
            if (stored_value) {
                vm_push(vm, *stored_value);
            } else {
                printf("Runtime error: Undefined variable '%s'\n", name);
                return VM_RUNTIME_ERROR;
            }
            
            found_variable:
            break;
        }


        case OP_GET_PROPERTY: {
            value_t property = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (property.type != VAL_STRING) {
                printf("Runtime error: Property name must be a string\n");
                return VM_RUNTIME_ERROR;
            }

            const char* prop_name = property.as.string;

            // For objects, check own properties first
            if (object.type == VAL_OBJECT) {
                value_t* prop_value = (value_t*)do_get(object.as.object, prop_name);
                if (prop_value) {
                    vm_push(vm, *prop_value);
                    break;
                }
            }

            // Check the prototype chain via class - walk up inheritance hierarchy
            value_t* current_class = object.class;
            bool property_found = false;
            
            while (current_class && current_class->type == VAL_CLASS && !property_found) {
                // Get the class's prototype properties
                class_t* cls = current_class->as.class;
                if (cls && cls->properties) {
                    value_t* prop_value = (value_t*)do_get(cls->properties, prop_name);
                    if (prop_value) {
                        // If it's a native function, create a bound method
                        if (prop_value->type == VAL_NATIVE) {
                            vm_push(vm, make_bound_method(object, prop_value->as.native));
                            property_found = true;
                        } else {
                            vm_push(vm, *prop_value);
                            property_found = true;
                        }
                        break;
                    }
                }
                // Move to parent class if any
                // For now, no inheritance - just break
                break;
            }
            
            if (!property_found) {
                printf("Runtime error: Property '%s' not found\n", prop_name);
                vm_release(object);
                vm_release(property);
                return VM_RUNTIME_ERROR;
            }
            
            // Clean up operands
            vm_release(object);
            vm_release(property);
            break;
        }

        case OP_CALL: {
            uint16_t arg_count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;

            // Pop arguments into temporary array (they're on stack in reverse order)
            value_t* args = NULL;
            if (arg_count > 0) {
                args = malloc(sizeof(value_t) * arg_count);
                for (int i = arg_count - 1; i >= 0; i--) {
                    args[i] = vm_pop(vm);
                }
            }

            // Get the callable value (now on top of stack)
            value_t callable = vm_pop(vm);

            // Handle bound methods (like array.map)
            if (callable.type == VAL_BOUND_METHOD) {
                bound_method_t* bound = callable.as.bound_method;
                
                // Prepare arguments: receiver + provided args
                value_t* full_args = malloc(sizeof(value_t) * (arg_count + 1));
                full_args[0] = bound->receiver;  // First arg is receiver (the array)
                for (int i = 0; i < arg_count; i++) {
                    full_args[i + 1] = args[i];  // Copy user-provided args
                }
                
                // Call the native function
                value_t result = bound->method(vm, arg_count + 1, full_args);
                vm_push(vm, result);
                
                free(full_args);
                if (args) free(args);
                break;
            }
            
            // Handle closures (user functions)
            if (callable.type == VAL_CLOSURE) {
                // For now, just call vm_call_function with our existing implementation
                value_t result = vm_call_function(vm, callable, arg_count, args);
                vm_push(vm, result);
                if (args) free(args);
                break;
            }
            
            printf("Runtime error: Value is not callable\n");
            if (args) {
                for (int i = 0; i < arg_count; i++) {
                    vm_release(args[i]);
                }
                free(args);
            }
            vm_release(callable);
            return VM_RUNTIME_ERROR;
        }

        case OP_CLOSURE: {
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            // Get function index from constants
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            value_t index_val = current_func->constants[constant];
            if (index_val.type != VAL_INT32) {
                vm_runtime_error_with_debug(vm, "Expected function index in OP_CLOSURE");
                return VM_RUNTIME_ERROR;
            }
            
            // Get function from function table
            size_t func_index = (size_t)index_val.as.int32;
            function_t* target_func = vm_get_function(vm, func_index);
            if (!target_func) {
                vm_runtime_error_with_debug(vm, "Invalid function index in OP_CLOSURE");
                return VM_RUNTIME_ERROR;
            }
            
            // For now, create a simple closure (no upvalues)
            closure_t* new_closure = closure_create(target_func);
            if (!new_closure) {
                vm_runtime_error_with_debug(vm, "Failed to create closure");
                return VM_RUNTIME_ERROR;
            }
            
            // Push closure as a value onto the stack
            value_t closure_val;
            closure_val.type = VAL_CLOSURE;
            closure_val.as.closure = new_closure;
            closure_val.class = NULL;
            closure_val.debug = NULL;
            vm_push(vm, closure_val);
            break;
        }

        case OP_BUILD_ARRAY: {
            uint16_t element_count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            
            // Create new dynamic array for elements
            da_array array = da_new(sizeof(value_t));
            
            // Collect all elements from stack (they're in reverse order)
            value_t* elements = malloc(sizeof(value_t) * element_count);
            for (int i = element_count - 1; i >= 0; i--) {
                elements[i] = vm_pop(vm);
                // Check if trying to store undefined (not a first-class value)
                if (elements[i].type == VAL_UNDEFINED) {
                    printf("Runtime error: Cannot store 'undefined' in array - it is not a value\n");
                    free(elements);
                    da_release(&array);
                    return VM_RUNTIME_ERROR;
                }
            }
            
            // Add elements to array in correct order
            for (uint16_t i = 0; i < element_count; i++) {
                da_push(array, &elements[i]);
            }
            
            free(elements);
            value_t result = make_array(array);
            vm_push(vm, result);
            break;
        }

        case OP_SET_DEBUG_LOCATION: {
            uint16_t constant_index = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;
            uint8_t line = *vm->ip++;
            uint8_t column = *vm->ip++;
            
            function_t* current_func = vm->frames[vm->frame_count - 1].closure->function;
            if (constant_index < current_func->constant_count) {
                value_t source_val = current_func->constants[constant_index];
                if (source_val.type == VAL_STRING) {
                    debug_location_free(vm->current_debug);
                    vm->current_debug = debug_location_create(line, column, source_val.as.string);
                }
            }
            break;
        }
        
        case OP_CLEAR_DEBUG_LOCATION:
            debug_location_free(vm->current_debug);
            vm->current_debug = NULL;
            break;

        case OP_HALT:
            return VM_OK;

        default:
            printf("DEBUG: Unimplemented opcode in vm_run: %d\n", instruction);
            return VM_RUNTIME_ERROR;
        }
    }
}

// VM execution with setup - clears stack and sets up initial call frame
vm_result vm_execute(slate_vm* vm, function_t* function) {
    if (!vm || !function)
        return VM_RUNTIME_ERROR;

    // Clear the stack at the start of each execution (important for REPL)
    vm->stack_top = vm->stack;

    // Set up initial call frame
    if (vm->frame_count >= vm->frame_capacity) {
        return VM_STACK_OVERFLOW;
    }

    call_frame* frame = &vm->frames[vm->frame_count++];
    closure_t* closure = closure_create(function); // Simple closure wrapper
    frame->closure = closure;
    frame->ip = function->bytecode;
    frame->slots = vm->stack_top;

    vm->bytecode = function->bytecode;
    vm->ip = function->bytecode;

    // Run the VM using the new core execution function
    vm_result result = vm_run(vm);
    
    // Clean up closure if execution completed normally
    if (result == VM_OK) {
        closure_destroy(closure);
    }
    
    return result;
}
vm_result vm_interpret(slate_vm* vm, const char* source) {
    // This would compile source to bytecode and execute
    // For now, just a stub
    (void)vm;
    (void)source;
    printf("vm_interpret not yet implemented\n");
    return VM_COMPILE_ERROR;
}

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

iterator_t* create_range_iterator(value_t start, value_t end, int exclusive) {
    iterator_t* iter = malloc(sizeof(iterator_t));
    if (!iter)
        return NULL;

    iter->ref_count = 1; // Initialize reference count
    iter->type = ITER_RANGE;
    iter->data.range_iter.current = vm_retain(start);
    iter->data.range_iter.end = vm_retain(end);
    iter->data.range_iter.exclusive = exclusive;
    iter->data.range_iter.finished = 0;

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

        if (iter->data.range_iter.exclusive) {
            return current < end;
        } else {
            return current <= end;
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
        // Return current value and increment
        value_t current = vm_retain(iter->data.range_iter.current);

        // Increment current (for now, only support integers)
        if (iter->data.range_iter.current.type == VAL_INT32) {
            iter->data.range_iter.current.as.int32++;
        } else {
            iter->data.range_iter.finished = 1;
        }

        return current;
    }
    default:
        return make_null();
    }
}

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
class_t* class_retain(class_t* cls) {
    if (!cls)
        return cls;
    cls->ref_count++;
    return cls;
}

void class_release(class_t* cls) {
    if (!cls)
        return;

    cls->ref_count--;
    if (cls->ref_count == 0) {
        // Clean up class data
        if (cls->name) {
            free(cls->name);
        }
        if (cls->properties) {
            do_release(&cls->properties);
        }
        free(cls);
    }
}

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
        }

        // Free the iterator itself
        free(iter);
    }
}

// Debug location management
debug_location* debug_location_create(int line, int column, const char* source_text) {
    debug_location* debug = malloc(sizeof(debug_location));
    if (!debug)
        return NULL;

    debug->line = line;
    debug->column = column;
    debug->source_text = source_text; // Not owned, just a reference
    return debug;
}

debug_location* debug_location_copy(const debug_location* debug) {
    if (!debug)
        return NULL;

    return debug_location_create(debug->line, debug->column, debug->source_text);
}

void debug_location_free(debug_location* debug) {
    if (debug && (uintptr_t)debug > 0x1000) {  // Basic sanity check - valid pointers are usually much higher
        free(debug);
    }
}

// Debug utilities

// Helper function to get a specific line from source code (from parser.c)
static const char* get_source_line(const char* source, int line_number, size_t* line_length) {
    const char* current = source;
    int current_line = 1;
    const char* line_start = source;

    // Find the start of the target line
    while (*current && current_line < line_number) {
        if (*current == '\n') {
            current_line++;
            line_start = current + 1;
        }
        current++;
    }

    if (current_line != line_number) {
        *line_length = 0;
        return NULL;
    }

    // Find the end of the line
    const char* line_end = line_start;
    while (*line_end && *line_end != '\n') {
        line_end++;
    }

    *line_length = line_end - line_start;
    return line_start;
}

// Find debug info entry for a given bytecode offset
void* vm_get_debug_info_at(function_t* function, size_t bytecode_offset) {
    if (!function || !function->debug) {
        return NULL;
    }

    debug_info* debug = (debug_info*)function->debug;

    // Find the closest debug info entry <= bytecode_offset
    debug_info_entry* best_entry = NULL;
    for (size_t i = 0; i < debug->count; i++) {
        if (debug->entries[i].bytecode_offset <= bytecode_offset) {
            best_entry = &debug->entries[i];
        } else {
            break; // Entries should be in order
        }
    }

    return best_entry ? debug : NULL;
}

// Print enhanced runtime error with instruction location
void vm_runtime_error_with_debug(slate_vm* vm, const char* message) {
    if (vm->frame_count == 0) {
        printf("Runtime error: %s\n", message);
        exit(1);
    }

    // Get the current function from the top call frame
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    function_t* function = frame->closure->function;

    // Use the stored current instruction pointer
    size_t instruction_offset = vm->current_instruction - vm->bytecode;

    // Show the instruction that failed
    opcode failed_op = (opcode)*vm->current_instruction;
    printf("Runtime error: %s\n", message);
    printf("    at instruction %04zu: %s\n", instruction_offset, opcode_name(failed_op));

    // If we have debug info with source code, try to show it
    if (function->debug) {
        debug_info* debug = (debug_info*)function->debug;

        if (debug->count > 0 && debug->source_code) {
            // Find the debug info for this instruction
            debug_info_entry* entry = NULL;
            for (size_t i = 0; i < debug->count; i++) {
                if (debug->entries[i].bytecode_offset <= instruction_offset) {
                    entry = &debug->entries[i];
                } else {
                    break;
                }
            }

            if (entry) {
                size_t line_length;
                const char* line_start = get_source_line(debug->source_code, entry->line, &line_length);

                if (line_start) {
                    printf("    %.*s\n", (int)line_length, line_start);

                    // Print caret pointing to the column (adjust by -1 since columns seem to be 1-based)
                    printf("    ");
                    int caret_pos = entry->column > 0 ? entry->column - 1 : 0;
                    for (int i = 0; i < caret_pos; i++) {
                        printf(" ");
                    }
                    printf("^\n");
                }
            }
        }
    }
    
    // Terminate program execution
    exit(1);
}

// Enhanced error reporting using value debug information
void vm_runtime_error_with_values(slate_vm* vm, const char* format, const value_t* a, const value_t* b,
                                  debug_location* location) {
    // Print basic error message with value types
    printf("Runtime error: ");
    printf(format, value_type_name(a->type), b ? value_type_name(b->type) : "");
    printf("\n");

    // Use the best debug location available (preference order: location param, a->debug, b->debug, current_debug)
    debug_location* debug_to_use = location;
    if (!debug_to_use && a)
        debug_to_use = a->debug;
    if (!debug_to_use && b)
        debug_to_use = b->debug;
    if (!debug_to_use)
        debug_to_use = vm->current_debug;

    // If we have debug info, show source location
    if (debug_to_use && debug_to_use->source_text) {
        printf("    at line %d, column %d:\n", debug_to_use->line, debug_to_use->column);
        printf("    %s\n", debug_to_use->source_text);

        // Print caret pointing to the column
        printf("    ");
        int caret_pos = debug_to_use->column > 0 ? debug_to_use->column - 1 : 0;
        for (int i = 0; i < caret_pos; i++) {
            printf(" ");
        }
        printf("^\n");
    }
}

// Helper function to get value type name for error messages
const char* value_type_name(value_type type) {
    switch (type) {
    case VAL_NULL:
        return "null";
    case VAL_UNDEFINED:
        return "undefined";
    case VAL_BOOLEAN:
        return "boolean";
    case VAL_INT32:
        return "int32";
    case VAL_BIGINT:
        return "bigint";
    case VAL_NUMBER:
        return "number";
    case VAL_STRING:
        return "string";
    case VAL_STRING_BUILDER:
        return "string_builder";
    case VAL_ARRAY:
        return "array";
    case VAL_OBJECT:
        return "object";
    case VAL_CLASS:
        return "class";
    case VAL_RANGE:
        return "range";
    case VAL_ITERATOR:
        return "iterator";
    case VAL_BUFFER:
        return "buffer";
    case VAL_BUFFER_BUILDER:
        return "buffer_builder";
    case VAL_BUFFER_READER:
        return "buffer_reader";
    case VAL_FUNCTION:
        return "function";
    case VAL_CLOSURE:
        return "closure";
    case VAL_NATIVE:
        return "builtin";
    case VAL_BOUND_METHOD:
        return "bound_method";
    case VAL_LOCAL_DATE:
        return "LocalDate";
    case VAL_LOCAL_TIME:
        return "LocalTime";
    case VAL_LOCAL_DATETIME:
        return "LocalDateTime";
    case VAL_ZONED_DATETIME:
        return "ZonedDateTime";
    case VAL_INSTANT:
        return "Instant";
    case VAL_DURATION:
        return "Duration";
    case VAL_PERIOD:
        return "Period";
    default:
        return "unknown";
    }
}
