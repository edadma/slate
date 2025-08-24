#include "vm.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h" // For debug_info functions

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
bit_vm* vm_create(void) {
    bit_vm* vm = malloc(sizeof(bit_vm));
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

    vm_reset(vm);
    return vm;
}

void vm_destroy(bit_vm* vm) {
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
    free(vm);
}

void vm_reset(bit_vm* vm) {
    if (!vm)
        return;

    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}

// Stack operations
void vm_push(bit_vm* vm, value_t value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = value;
    vm->stack_top++;
}

value_t vm_pop(bit_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

value_t vm_peek(bit_vm* vm, int distance) { return vm->stack_top[-1 - distance]; }

// Value creation functions
value_t make_null(void) {
    value_t value;
    value.type = VAL_NULL;
    return value;
}

value_t make_undefined(void) {
    value_t value;
    value.type = VAL_UNDEFINED;
    return value;
}

value_t make_boolean(int val) {
    value_t value;
    value.type = VAL_BOOLEAN;
    value.as.boolean = val;
    return value;
}

value_t make_number(double val) {
    value_t value;
    value.type = VAL_NUMBER;
    value.as.number = val;
    return value;
}

value_t make_string(const char* val) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = ds_new(val); // Using dynamic_string.h!
    return value;
}

value_t make_string_ds(ds_string str) {
    value_t value;
    value.type = VAL_STRING;
    value.as.string = str; // Take ownership of the ds_string
    return value;
}

value_t make_array(da_array array) {
    value_t value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    return value;
}

value_t make_object(do_object object) {
    value_t value;
    value.type = VAL_OBJECT;
    value.as.object = object;
    return value;
}

value_t make_function(function_t* function) {
    value_t value;
    value.type = VAL_FUNCTION;
    value.as.function = function;
    return value;
}

value_t make_closure(closure_t* closure) {
    value_t value;
    value.type = VAL_CLOSURE;
    value.as.closure = closure;
    return value;
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
    case VAL_NUMBER:
        return value.as.number == 0.0;
    case VAL_STRING:
        return value.as.string == NULL || strlen(value.as.string) == 0;
    default:
        return 0;
    }
}

int values_equal(value_t a, value_t b) {
    if (a.type != b.type)
        return 0;

    switch (a.type) {
    case VAL_NULL:
        return 1;
    case VAL_UNDEFINED:
        return 1;
    case VAL_BOOLEAN:
        return a.as.boolean == b.as.boolean;
    case VAL_NUMBER:
        return a.as.number == b.as.number;
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
    case VAL_FUNCTION:
        return a.as.function == b.as.function;
    case VAL_CLOSURE:
        return a.as.closure == b.as.closure;
    default:
        return 0;
    }
}

void print_value(value_t value) {
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
    case VAL_NUMBER:
        printf("%.6g", value.as.number);
        break;
    case VAL_STRING:
        printf("\"%s\"", value.as.string ? value.as.string : ""); // DS strings work directly!
        break;
    case VAL_ARRAY: {
        printf("[");
        if (value.as.array) {
            int length = da_length(value.as.array);
            for (int i = 0; i < length; i++) {
                if (i > 0)
                    printf(", ");
                value_t* element = (value_t*)da_get(value.as.array, i);
                if (element) {
                    print_value(*element);
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
    case VAL_FUNCTION:
        printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
        break;
    case VAL_CLOSURE:
        printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
        break;
    }
}

void free_value(value_t value) {
    switch (value.type) {
    case VAL_NULL:
    case VAL_UNDEFINED:
    case VAL_BOOLEAN:
    case VAL_NUMBER:
        // No cleanup needed for these types
        break;
    case VAL_STRING: {
        ds_string temp = value.as.string;
        ds_release(&temp); // DS cleanup with reference counting!
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
    case VAL_FUNCTION:
        function_destroy(value.as.function);
        break;
    case VAL_CLOSURE:
        closure_destroy(value.as.closure);
        break;
    default:
        // No cleanup needed for basic types
        break;
    }
}

// Constant pool management
size_t vm_add_constant(bit_vm* vm, value_t value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

value_t vm_get_constant(bit_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
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
    case OP_ADD:
        return "ADD";
    case OP_SUBTRACT:
        return "SUBTRACT";
    case OP_MULTIPLY:
        return "MULTIPLY";
    case OP_DIVIDE:
        return "DIVIDE";
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
    case OP_HALT:
        return "HALT";
    default:
        return "UNKNOWN";
    }
}

// Basic VM execution (stub for now)
vm_result vm_execute(bit_vm* vm, function_t* function) {
    if (!vm || !function)
        return VM_RUNTIME_ERROR;

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

    // Basic execution loop (simplified)
    for (;;) {
        opcode instruction = (opcode)*vm->ip++;

        switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2; // Skip the operand bytes
            vm_push(vm, function->constants[constant]);
            break;
        }

        case OP_PUSH_NULL:
            vm_push(vm, make_null());
            break;

        case OP_PUSH_UNDEFINED:
            vm_push(vm, make_undefined());
            break;

        case OP_PUSH_TRUE:
            vm_push(vm, make_boolean(1));
            break;

        case OP_PUSH_FALSE:
            vm_push(vm, make_boolean(0));
            break;

        case OP_POP: {
            value_t popped = vm_pop(vm);
            // Store in a global for the main function to see
            if (vm->stack_top == vm->stack) {
                // If stack becomes empty, push the value back so main can see it
                vm_push(vm, popped);
            }
            break;
        }


        case OP_ADD: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);

            // Numeric addition
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
                vm_push(vm, make_number(a.as.number + b.as.number));
            }
            // String concatenation (if either operand is a string)
            else if (a.type == VAL_STRING || b.type == VAL_STRING) {
                // Convert both to strings
                ds_string str_a, str_b;

                if (a.type == VAL_STRING) {
                    str_a = ds_retain(a.as.string);
                } else if (a.type == VAL_NUMBER) {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%.6g", a.as.number);
                    str_a = ds_new(buffer);
                } else if (a.type == VAL_BOOLEAN) {
                    str_a = ds_new(a.as.boolean ? "true" : "false");
                } else if (a.type == VAL_UNDEFINED) {
                    str_a = ds_new("undefined");
                } else {
                    str_a = ds_new("null");
                }

                if (b.type == VAL_STRING) {
                    str_b = ds_retain(b.as.string);
                } else if (b.type == VAL_NUMBER) {
                    char buffer[32];
                    snprintf(buffer, sizeof(buffer), "%.6g", b.as.number);
                    str_b = ds_new(buffer);
                } else if (b.type == VAL_BOOLEAN) {
                    str_b = ds_new(b.as.boolean ? "true" : "false");
                } else if (b.type == VAL_UNDEFINED) {
                    str_b = ds_new("undefined");
                } else {
                    str_b = ds_new("null");
                }

                // Concatenate using DS library
                ds_string result = ds_append(str_a, str_b);
                vm_push(vm, make_string_ds(result));

                // Clean up temporary strings
                ds_release(&str_a);
                ds_release(&str_b);
            } else {
                vm_runtime_error_with_debug(vm, "Can't add non-numeric values");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }


        case OP_SUBTRACT: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
                vm_push(vm, make_number(a.as.number - b.as.number));
            } else {
                printf("Runtime error: Cannot subtract non-numeric values\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_MULTIPLY: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
                vm_push(vm, make_number(a.as.number * b.as.number));
            } else {
                printf("Runtime error: Cannot multiply non-numeric values\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_DIVIDE: {
            value_t b = vm_pop(vm);
            value_t a = vm_pop(vm);
            if (a.type == VAL_NUMBER && b.type == VAL_NUMBER) {
                if (b.as.number == 0) {
                    printf("Runtime error: Division by zero\n");
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                vm_push(vm, make_number(a.as.number / b.as.number));
            } else {
                printf("Runtime error: Cannot divide non-numeric values\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_NEGATE: {
            value_t a = vm_pop(vm);
            if (a.type == VAL_NUMBER) {
                vm_push(vm, make_number(-a.as.number));
            } else {
                printf("Runtime error: Cannot negate non-numeric value\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_BUILD_ARRAY: {
            uint16_t count = *vm->ip | (*(vm->ip + 1) << 8);
            vm->ip += 2;

            // Create new dynamic array for bit_value elements
            da_array array = da_new(sizeof(value_t));

            // Collect all elements from stack (they're in reverse order)
            value_t* elements = malloc(sizeof(value_t) * count);
            for (int i = count - 1; i >= 0; i--) {
                elements[i] = vm_pop(vm);
            }

            // Add elements to array in correct order
            for (size_t i = 0; i < count; i++) {
                da_push(array, &elements[i]);
            }
            free(elements);

            vm_push(vm, make_array(array));
            break;
        }

        case OP_GET_INDEX: {
            value_t index = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (object.type == VAL_ARRAY && index.type == VAL_NUMBER) {
                int idx = (int)index.as.number;
                // Check bounds - out of bounds is an error
                if (idx < 0 || idx >= da_length(object.as.array)) {
                    printf("Runtime error: Array index %d out of bounds (length: %d)\n", idx,
                           da_length(object.as.array));
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t* element = (value_t*)da_get(object.as.array, idx);
                vm_push(vm, *element);
            } else if (object.type == VAL_STRING && index.type == VAL_NUMBER) {
                // String indexing
                int idx = (int)index.as.number;
                ds_string str = object.as.string;
                if (idx < 0 || idx >= (int)ds_length(str)) {
                    printf("Runtime error: String index %d out of bounds (length: %d)\n", idx, (int)ds_length(str));
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                char ch = str[idx]; // ds_string is just char*, can index directly
                char result[2] = {ch, '\0'};
                vm_push(vm, make_string(result));
            } else {
                printf("Runtime error: Cannot index non-array/string value\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_SET_INDEX: {
            value_t value = vm_pop(vm);
            value_t index = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (object.type == VAL_ARRAY && index.type == VAL_NUMBER) {
                size_t idx = (size_t)index.as.number;
                // Set element in dynamic array
                da_set(object.as.array, (int)idx, &value);
                vm_push(vm, value); // Assignment returns the assigned value
            } else {
                printf("Runtime error: Cannot set index on non-array value\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
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

            // Arrays: act as functions from index to value
            if (callable.type == VAL_ARRAY) {
                if (arg_count != 1) {
                    printf("Runtime error: Array indexing requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t index = args[0];
                if (index.type != VAL_NUMBER) {
                    printf("Runtime error: Array index must be a number\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                int idx = (int)index.as.number;
                // Check bounds - out of bounds is an error
                if (idx < 0 || idx >= da_length(callable.as.array)) {
                    printf("Runtime error: Array index %d out of bounds (length: %d)\n", idx,
                           da_length(callable.as.array));
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t* element = (value_t*)da_get(callable.as.array, idx);
                vm_push(vm, *element);
                if (args)
                    free(args);
            }
            // Strings: act as functions from index to character
            else if (callable.type == VAL_STRING) {
                if (arg_count != 1) {
                    printf("Runtime error: String indexing requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t index = args[0];
                if (index.type != VAL_NUMBER) {
                    printf("Runtime error: String index must be a number\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                int idx = (int)index.as.number;
                ds_string str = callable.as.string;
                if (idx < 0 || idx >= (int)ds_length(str)) {
                    printf("Runtime error: String index %d out of bounds (length: %d)\n", idx, (int)ds_length(str));
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                char ch = str[idx];
                char result[2] = {ch, '\0'};
                vm_push(vm, make_string(result));
                if (args)
                    free(args);
            }
            // Objects: act as functions from property name to value
            else if (callable.type == VAL_OBJECT) {
                if (arg_count != 1) {
                    printf("Runtime error: Object property access requires exactly 1 argument\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                value_t key = args[0];
                if (key.type != VAL_STRING) {
                    printf("Runtime error: Object key must be a string\n");
                    if (args)
                        free(args);
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                // Get property from dynamic object (returns bit_value*)
                value_t* prop_value = (value_t*)do_get(callable.as.object, key.as.string);
                if (prop_value) {
                    vm_push(vm, *prop_value);
                } else {
                    // Missing property returns undefined
                    vm_push(vm, make_undefined());
                }
                if (args)
                    free(args);
            }
            // Functions: traditional function call (not implemented yet)
            else if (callable.type == VAL_FUNCTION || callable.type == VAL_CLOSURE) {
                printf("Runtime error: Function calls not yet implemented\n");
                if (args)
                    free(args);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            } else {
                printf("Runtime error: Value is not callable\n");
                if (args)
                    free(args);
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }
            break;
        }

        case OP_GET_PROPERTY: {
            value_t property = vm_pop(vm);
            value_t object = vm_pop(vm);

            if (property.type != VAL_STRING) {
                printf("Runtime error: Property name must be a string\n");
                vm->frame_count--;
                closure_destroy(closure);
                return VM_RUNTIME_ERROR;
            }

            const char* prop_name = property.as.string;

            if (object.type == VAL_ARRAY) {
                if (strcmp(prop_name, "length") == 0) {
                    vm_push(vm, make_number((double)da_length(object.as.array)));
                } else {
                    // Invalid property returns undefined, not error
                    vm_push(vm, make_undefined());
                }
            } else if (object.type == VAL_STRING) {
                if (strcmp(prop_name, "length") == 0) {
                    vm_push(vm, make_number((double)ds_length(object.as.string)));
                } else {
                    // Invalid property returns undefined, not error
                    vm_push(vm, make_undefined());
                }
            } else if (object.type == VAL_OBJECT) {
                // Get property from dynamic object (returns bit_value*)
                value_t* prop_value = (value_t*)do_get(object.as.object, prop_name);
                if (prop_value) {
                    vm_push(vm, *prop_value);
                } else {
                    // Missing property returns undefined
                    vm_push(vm, make_undefined());
                }
            } else {
                // Property access on invalid types returns undefined
                vm_push(vm, make_undefined());
            }
            break;
        }

        case OP_HALT:
            vm->frame_count--;
            closure_destroy(closure);
            return VM_OK;

        default:
            printf("Unknown instruction: %s\n", opcode_name(instruction));
            vm->frame_count--;
            closure_destroy(closure);
            return VM_RUNTIME_ERROR;
        }
    }
}

vm_result vm_interpret(bit_vm* vm, const char* source) {
    // This would compile source to bytecode and execute
    // For now, just a stub
    (void)vm;
    (void)source;
    printf("vm_interpret not yet implemented\n");
    return VM_COMPILE_ERROR;
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

// Print enhanced runtime error with source location
void vm_runtime_error_with_debug(bit_vm* vm, const char* message) {
    if (vm->frame_count == 0) {
        printf("Runtime error: %s\n", message);
        return;
    }

    // Get the current function from the top call frame
    call_frame* frame = &vm->frames[vm->frame_count - 1];
    function_t* function = frame->closure->function;

    // Use VM's instruction pointer for the offset calculation
    size_t instruction_offset = vm->ip - vm->bytecode - 1; // -1 because ip was advanced after executing

    if (function->debug) {
        debug_info* debug = (debug_info*)function->debug;

        // Find the debug info for this instruction
        debug_info_entry* entry = NULL;
        for (size_t i = 0; i < debug->count; i++) {
            if (debug->entries[i].bytecode_offset <= instruction_offset) {
                entry = &debug->entries[i];
            } else {
                break;
            }
        }

        if (entry && debug->source_code) {
            // Print enhanced error with source line
            printf("Runtime error: %s\n", message);

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
        } else {
            printf("Runtime error: %s\n", message);
        }
    } else {
        printf("Runtime error: %s\n", message);
    }
}
