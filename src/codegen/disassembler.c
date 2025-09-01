#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Debug functions
void chunk_disassemble(bytecode_chunk* chunk, const char* name) {
    printf("== %s ==\n", name);
    
    for (size_t offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
}

size_t disassemble_instruction(bytecode_chunk* chunk, size_t offset) {
    printf("%04zu ", offset);
    
    uint8_t instruction = chunk->code[offset];
    
    switch (instruction) {
        case OP_PUSH_CONSTANT: {
            uint16_t constant = chunk->code[offset + 1] | (chunk->code[offset + 2] << 8);
            printf("%-16s %4d ", opcode_name(instruction), constant);
            if (constant >= chunk->constant_count) {
                printf("'[INVALID INDEX - max: %zu]'\n", chunk->constant_count - 1);
            } else {
                printf("'");
                print_value(NULL, chunk->constants[constant]);
                printf("'\n");
            }
            return offset + 3;
        }
        
        case OP_BUILD_ARRAY:
        case OP_BUILD_OBJECT:
        case OP_CALL:
        case OP_CLOSURE:
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