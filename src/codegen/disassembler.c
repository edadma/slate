#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Debug functions
void chunk_disassemble(bytecode_chunk* chunk, const char* name) {
    chunk_disassemble_with_vm(chunk, name, NULL);
}

void chunk_disassemble_with_vm(bytecode_chunk* chunk, const char* name, vm_t* vm) {
    printf("== %s ==\n", name);
    
    for (size_t offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction_with_vm(chunk, offset, vm);
    }
}

size_t disassemble_instruction(bytecode_chunk* chunk, size_t offset) {
    return disassemble_instruction_with_vm(chunk, offset, NULL);
}

size_t disassemble_instruction_with_vm(bytecode_chunk* chunk, size_t offset, vm_t* vm) {
    printf("%04zu ", offset);
    
    uint8_t instruction = chunk->code[offset];
    
    switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d ", opcode_name(instruction), constant);
            if (constant >= chunk->constant_count) {
                printf("'[INVALID INDEX - max: %zu]'\n", chunk->constant_count - 1);
            } else {
                value_t value = chunk->constants[constant];
                printf("'");
                print_value(NULL, value);
                printf("'\n");
                
                // If this is a function, disassemble it too
                if (value.type == VAL_FUNCTION) {
                    printf("\n");
                    function_t* func = value.as.function;
                    bytecode_chunk func_chunk = {
                        .code = func->bytecode,
                        .count = func->bytecode_length,
                        .constants = func->constants,
                        .constant_count = func->constant_count
                    };
                    chunk_disassemble_with_vm(&func_chunk, func->name ? func->name : "<anonymous>", vm);
                    printf("\n");
                }
            }
            return offset + 3;
        }
        
        case OP_CLOSURE: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d ", opcode_name(instruction), constant);
            if (constant >= chunk->constant_count) {
                printf("'[INVALID INDEX - max: %zu]'\n", chunk->constant_count - 1);
            } else {
                value_t index_val = chunk->constants[constant];
                printf("'");
                print_value(NULL, index_val);
                printf("'\n");
                
                // For closure, the constant is a function index
                if (index_val.type == VAL_INT32 && vm) {
                    int32_t func_index = index_val.as.int32;
                    printf("                     (function index: %d)\n", func_index);
                    
                    // Disassemble the function if we have VM access
                    if (func_index >= 0 && (size_t)func_index < da_length(vm->functions)) {
                        function_t* func = da_get(vm->functions, func_index);
                        if (func) {
                            printf("\n");
                            bytecode_chunk func_chunk = {
                                .code = func->bytecode,
                                .count = func->bytecode_length,
                                .constants = func->constants,
                                .constant_count = func->constant_count
                            };
                            chunk_disassemble_with_vm(&func_chunk, func->name ? func->name : "<anonymous>", vm);
                            printf("\n");
                        }
                    }
                } else if (index_val.type == VAL_INT32) {
                    printf("                     (function index: %d)\n", index_val.as.int32);
                }
            }
            return offset + 3;
        }
        
        case OP_BUILD_ARRAY:
        case OP_BUILD_OBJECT:
        case OP_CALL:
        case OP_GET_GLOBAL:
        case OP_SET_GLOBAL:
        case OP_DEFINE_GLOBAL:
        case OP_JUMP:
        case OP_JUMP_IF_FALSE:
        case OP_JUMP_IF_TRUE:
        case OP_LOOP: {
            uint16_t operand = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d\n", opcode_name(instruction), operand);
            return offset + 3;
        }
        
        case OP_SET_DEBUG_LOCATION: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            uint8_t line = chunk->code[offset + 3];
            uint8_t column = chunk->code[offset + 4];
            printf("%-16s %4d (line %d, col %d)\n", opcode_name(instruction), constant, line, column);
            return offset + 5;
        }
        
        case OP_CLEAR_DEBUG_LOCATION:
            printf("%-16s\n", opcode_name(instruction));
            return offset + 1;
        
        default:
            printf("%s\n", opcode_name(instruction));
            return offset + 1;
    }
}