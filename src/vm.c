#define DS_IMPLEMENTATION  // Define implementation here
#include "vm.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>

#define STACK_MAX 256
#define FRAMES_MAX 64
#define CONSTANTS_MAX 256

// VM lifecycle functions
bit_vm* vm_create(void) {
    bit_vm* vm = malloc(sizeof(bit_vm));
    if (!vm) return NULL;
    
    vm->stack = malloc(sizeof(bit_value) * STACK_MAX);
    vm->frames = malloc(sizeof(call_frame) * FRAMES_MAX);
    vm->constants = malloc(sizeof(bit_value) * CONSTANTS_MAX);
    
    if (!vm->stack || !vm->frames || !vm->constants) {
        vm_destroy(vm);
        return NULL;
    }
    
    vm->stack_capacity = STACK_MAX;
    vm->frame_capacity = FRAMES_MAX;
    vm->constant_capacity = CONSTANTS_MAX;
    
    vm->globals = object_create();
    if (!vm->globals) {
        vm_destroy(vm);
        return NULL;
    }
    
    vm_reset(vm);
    return vm;
}

void vm_destroy(bit_vm* vm) {
    if (!vm) return;
    
    // Free constants
    for (size_t i = 0; i < vm->constant_count; i++) {
        free_value(vm->constants[i]);
    }
    
    free(vm->stack);
    free(vm->frames);
    free(vm->constants);
    object_destroy(vm->globals);
    free(vm);
}

void vm_reset(bit_vm* vm) {
    if (!vm) return;
    
    vm->stack_top = vm->stack;
    vm->frame_count = 0;
    vm->constant_count = 0;
    vm->bytes_allocated = 0;
    vm->bytecode = NULL;
    vm->ip = NULL;
}

// Stack operations
void vm_push(bit_vm* vm, bit_value value) {
    assert(vm->stack_top - vm->stack < vm->stack_capacity);
    *vm->stack_top = value;
    vm->stack_top++;
}

bit_value vm_pop(bit_vm* vm) {
    assert(vm->stack_top > vm->stack);
    vm->stack_top--;
    return *vm->stack_top;
}

bit_value vm_peek(bit_vm* vm, int distance) {
    return vm->stack_top[-1 - distance];
}

// Value creation functions
bit_value make_null(void) {
    bit_value value;
    value.type = VAL_NULL;
    return value;
}

bit_value make_boolean(int val) {
    bit_value value;
    value.type = VAL_BOOLEAN;
    value.as.boolean = val;
    return value;
}

bit_value make_number(double val) {
    bit_value value;
    value.type = VAL_NUMBER;
    value.as.number = val;
    return value;
}

bit_value make_string(const char* val) {
    bit_value value;
    value.type = VAL_STRING;
    value.as.string = ds_new(val);  // Using dynamic_string.h!
    return value;
}

bit_value make_string_ds(ds_string str) {
    bit_value value;
    value.type = VAL_STRING;
    value.as.string = str;  // Take ownership of the ds_string
    return value;
}

bit_value make_array(bit_array* array) {
    bit_value value;
    value.type = VAL_ARRAY;
    value.as.array = array;
    return value;
}

bit_value make_object(bit_object* object) {
    bit_value value;
    value.type = VAL_OBJECT;
    value.as.object = object;
    return value;
}

bit_value make_function(bit_function* function) {
    bit_value value;
    value.type = VAL_FUNCTION;
    value.as.function = function;
    return value;
}

bit_value make_closure(bit_closure* closure) {
    bit_value value;
    value.type = VAL_CLOSURE;
    value.as.closure = closure;
    return value;
}

// Value utility functions
int is_falsy(bit_value value) {
    switch (value.type) {
        case VAL_NULL: return 1;
        case VAL_BOOLEAN: return !value.as.boolean;
        case VAL_NUMBER: return value.as.number == 0.0;
        case VAL_STRING: return value.as.string == NULL || strlen(value.as.string) == 0;
        default: return 0;
    }
}

int values_equal(bit_value a, bit_value b) {
    if (a.type != b.type) return 0;
    
    switch (a.type) {
        case VAL_NULL: return 1;
        case VAL_BOOLEAN: return a.as.boolean == b.as.boolean;
        case VAL_NUMBER: return a.as.number == b.as.number;
        case VAL_STRING: 
            if (a.as.string == NULL && b.as.string == NULL) return 1;
            if (a.as.string == NULL || b.as.string == NULL) return 0;
            return strcmp(a.as.string, b.as.string) == 0;  // DS strings work with strcmp!
        case VAL_ARRAY: return a.as.array == b.as.array;
        case VAL_OBJECT: return a.as.object == b.as.object;
        case VAL_FUNCTION: return a.as.function == b.as.function;
        case VAL_CLOSURE: return a.as.closure == b.as.closure;
        default: return 0;
    }
}

void print_value(bit_value value) {
    switch (value.type) {
        case VAL_NULL:
            printf("null");
            break;
        case VAL_BOOLEAN:
            printf(value.as.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%.6g", value.as.number);
            break;
        case VAL_STRING:
            printf("\"%s\"", value.as.string ? value.as.string : "");  // DS strings work directly!
            break;
        case VAL_ARRAY:
            printf("[Array]");
            break;
        case VAL_OBJECT:
            printf("{Object}");
            break;
        case VAL_FUNCTION:
            printf("<function %s>", value.as.function->name ? value.as.function->name : "anonymous");
            break;
        case VAL_CLOSURE:
            printf("<closure %s>", value.as.closure->function->name ? value.as.closure->function->name : "anonymous");
            break;
    }
}

void free_value(bit_value value) {
    switch (value.type) {
        case VAL_STRING: {
            ds_string temp = value.as.string;
            ds_release(&temp);  // DS cleanup with reference counting!
            break;
        }
        case VAL_ARRAY:
            array_destroy(value.as.array);
            break;
        case VAL_OBJECT:
            object_destroy(value.as.object);
            break;
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
size_t vm_add_constant(bit_vm* vm, bit_value value) {
    assert(vm->constant_count < vm->constant_capacity);
    vm->constants[vm->constant_count] = value;
    return vm->constant_count++;
}

bit_value vm_get_constant(bit_vm* vm, size_t index) {
    assert(index < vm->constant_count);
    return vm->constants[index];
}

// Object operations
bit_object* object_create(void) {
    bit_object* object = malloc(sizeof(bit_object));
    if (!object) return NULL;
    
    object->properties = NULL;
    object->count = 0;
    object->capacity = 0;
    
    return object;
}

void object_destroy(bit_object* object) {
    if (!object) return;
    
    for (size_t i = 0; i < object->count; i++) {
        free(object->properties[i].key);
        free_value(object->properties[i].value);
    }
    
    free(object->properties);
    free(object);
}

bit_value object_get(bit_object* object, const char* key) {
    if (!object || !key) return make_null();
    
    for (size_t i = 0; i < object->count; i++) {
        if (strcmp(object->properties[i].key, key) == 0) {
            return object->properties[i].value;
        }
    }
    
    return make_null();
}

void object_set(bit_object* object, const char* key, bit_value value) {
    if (!object || !key) return;
    
    // Check if property already exists
    for (size_t i = 0; i < object->count; i++) {
        if (strcmp(object->properties[i].key, key) == 0) {
            free_value(object->properties[i].value);
            object->properties[i].value = value;
            return;
        }
    }
    
    // Add new property
    if (object->count >= object->capacity) {
        size_t new_capacity = object->capacity == 0 ? 8 : object->capacity * 2;
        object->properties = realloc(object->properties, 
                                   sizeof(bit_property) * new_capacity);
        object->capacity = new_capacity;
    }
    
    object->properties[object->count].key = strdup(key);
    object->properties[object->count].value = value;
    object->count++;
}

// Array operations
bit_array* array_create(void) {
    bit_array* array = malloc(sizeof(bit_array));
    if (!array) return NULL;
    
    array->elements = NULL;
    array->count = 0;
    array->capacity = 0;
    
    return array;
}

void array_destroy(bit_array* array) {
    if (!array) return;
    
    for (size_t i = 0; i < array->count; i++) {
        free_value(array->elements[i]);
    }
    
    free(array->elements);
    free(array);
}

void array_push(bit_array* array, bit_value value) {
    if (!array) return;
    
    if (array->count >= array->capacity) {
        size_t new_capacity = array->capacity == 0 ? 8 : array->capacity * 2;
        array->elements = realloc(array->elements, sizeof(bit_value) * new_capacity);
        array->capacity = new_capacity;
    }
    
    array->elements[array->count++] = value;
}

bit_value array_get(bit_array* array, size_t index) {
    if (!array || index >= array->count) {
        return make_null();
    }
    
    return array->elements[index];
}

void array_set(bit_array* array, size_t index, bit_value value) {
    if (!array || index >= array->count) return;
    
    free_value(array->elements[index]);
    array->elements[index] = value;
}

// Function operations
bit_function* function_create(const char* name) {
    bit_function* function = malloc(sizeof(bit_function));
    if (!function) return NULL;
    
    function->bytecode = NULL;
    function->bytecode_length = 0;
    function->constants = NULL;
    function->constant_count = 0;
    function->parameter_names = NULL;
    function->parameter_count = 0;
    function->local_count = 0;
    function->name = name ? strdup(name) : NULL;
    
    return function;
}

void function_destroy(bit_function* function) {
    if (!function) return;
    
    free(function->bytecode);
    
    for (size_t i = 0; i < function->constant_count; i++) {
        free_value(function->constants[i]);
    }
    free(function->constants);
    
    for (size_t i = 0; i < function->parameter_count; i++) {
        free(function->parameter_names[i]);
    }
    free(function->parameter_names);
    
    free(function->name);
    free(function);
}

bit_closure* closure_create(bit_function* function) {
    bit_closure* closure = malloc(sizeof(bit_closure));
    if (!closure) return NULL;
    
    closure->function = function;
    closure->upvalues = NULL;
    closure->upvalue_count = 0;
    
    return closure;
}

void closure_destroy(bit_closure* closure) {
    if (!closure) return;
    
    function_destroy(closure->function);
    free(closure->upvalues);
    free(closure);
}

// Bytecode utilities
const char* opcode_name(opcode op) {
    switch (op) {
        case OP_PUSH_CONSTANT: return "PUSH_CONSTANT";
        case OP_PUSH_NULL: return "PUSH_NULL";
        case OP_PUSH_TRUE: return "PUSH_TRUE";
        case OP_PUSH_FALSE: return "PUSH_FALSE";
        case OP_POP: return "POP";
        case OP_DUP: return "DUP";
        case OP_ADD: return "ADD";
        case OP_SUBTRACT: return "SUBTRACT";
        case OP_MULTIPLY: return "MULTIPLY";
        case OP_DIVIDE: return "DIVIDE";
        case OP_NEGATE: return "NEGATE";
        case OP_EQUAL: return "EQUAL";
        case OP_NOT_EQUAL: return "NOT_EQUAL";
        case OP_LESS: return "LESS";
        case OP_LESS_EQUAL: return "LESS_EQUAL";
        case OP_GREATER: return "GREATER";
        case OP_GREATER_EQUAL: return "GREATER_EQUAL";
        case OP_NOT: return "NOT";
        case OP_AND: return "AND";
        case OP_OR: return "OR";
        case OP_GET_LOCAL: return "GET_LOCAL";
        case OP_SET_LOCAL: return "SET_LOCAL";
        case OP_GET_GLOBAL: return "GET_GLOBAL";
        case OP_SET_GLOBAL: return "SET_GLOBAL";
        case OP_DEFINE_GLOBAL: return "DEFINE_GLOBAL";
        case OP_GET_PROPERTY: return "GET_PROPERTY";
        case OP_SET_PROPERTY: return "SET_PROPERTY";
        case OP_GET_INDEX: return "GET_INDEX";
        case OP_SET_INDEX: return "SET_INDEX";
        case OP_BUILD_ARRAY: return "BUILD_ARRAY";
        case OP_BUILD_OBJECT: return "BUILD_OBJECT";
        case OP_CLOSURE: return "CLOSURE";
        case OP_CALL: return "CALL";
        case OP_RETURN: return "RETURN";
        case OP_JUMP: return "JUMP";
        case OP_JUMP_IF_FALSE: return "JUMP_IF_FALSE";
        case OP_JUMP_IF_TRUE: return "JUMP_IF_TRUE";
        case OP_LOOP: return "LOOP";
        case OP_HALT: return "HALT";
        default: return "UNKNOWN";
    }
}

// Basic VM execution (stub for now)
vm_result vm_execute(bit_vm* vm, bit_function* function) {
    if (!vm || !function) return VM_RUNTIME_ERROR;
    
    // Set up initial call frame
    if (vm->frame_count >= vm->frame_capacity) {
        return VM_STACK_OVERFLOW;
    }
    
    call_frame* frame = &vm->frames[vm->frame_count++];
    bit_closure* closure = closure_create(function);  // Simple closure wrapper
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
                
            case OP_PUSH_TRUE:
                vm_push(vm, make_boolean(1));
                break;
                
            case OP_PUSH_FALSE:
                vm_push(vm, make_boolean(0));
                break;
                
            case OP_POP: {
                bit_value popped = vm_pop(vm);
                // Store in a global for the main function to see
                if (vm->stack_top == vm->stack) {
                    // If stack becomes empty, push the value back so main can see it
                    vm_push(vm, popped);
                }
                break;
            }
                
            case OP_ADD: {
                bit_value b = vm_pop(vm);
                bit_value a = vm_pop(vm);
                
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
                    printf("Runtime error: Cannot add these value types\n");
                    vm->frame_count--;
                    closure_destroy(closure);
                    return VM_RUNTIME_ERROR;
                }
                break;
            }
            
            case OP_SUBTRACT: {
                bit_value b = vm_pop(vm);
                bit_value a = vm_pop(vm);
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
                bit_value b = vm_pop(vm);
                bit_value a = vm_pop(vm);
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
                bit_value b = vm_pop(vm);
                bit_value a = vm_pop(vm);
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
                bit_value a = vm_pop(vm);
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