#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Bytecode chunk functions
bytecode_chunk* chunk_create(void) {
    bytecode_chunk* chunk = malloc(sizeof(bytecode_chunk));
    if (!chunk) return NULL;
    
    chunk->code = NULL;
    chunk->count = 0;
    chunk->capacity = 0;
    chunk->constants = NULL;
    chunk->constant_count = 0;
    chunk->constant_capacity = 0;
    chunk->debug = NULL; // No debug info by default
    
    return chunk;
}

bytecode_chunk* chunk_create_with_debug(const char* source_code) {
    bytecode_chunk* chunk = chunk_create();
    if (!chunk) return NULL;
    
    chunk->debug = debug_info_create(source_code);
    if (!chunk->debug) {
        chunk_destroy(chunk);
        return NULL;
    }
    
    return chunk;
}

void chunk_destroy(bytecode_chunk* chunk) {
    if (!chunk) return;
    
    free(chunk->code);
    
    for (size_t i = 0; i < chunk->constant_count; i++) {
        free_value(chunk->constants[i]);
    }
    free(chunk->constants);
    
    debug_info_destroy(chunk->debug);
    
    free(chunk);
}

void chunk_write_byte(bytecode_chunk* chunk, uint8_t byte) {
    if (chunk->count >= chunk->capacity) {
        size_t new_capacity = chunk->capacity == 0 ? 8 : chunk->capacity * 2;
        chunk->code = realloc(chunk->code, new_capacity);
        chunk->capacity = new_capacity;
    }
    
    chunk->code[chunk->count++] = byte;
}

void chunk_write_opcode(bytecode_chunk* chunk, opcode op) {
    chunk_write_byte(chunk, (uint8_t)op);
}

void chunk_write_operand(bytecode_chunk* chunk, uint16_t operand) {
    chunk_write_byte(chunk, (uint8_t)(operand & 0xFF));
    chunk_write_byte(chunk, (uint8_t)((operand >> 8) & 0xFF));
}

size_t chunk_add_constant(bytecode_chunk* chunk, value_t value) {
    if (chunk->constant_count >= chunk->constant_capacity) {
        size_t new_capacity = chunk->constant_capacity == 0 ? 8 : chunk->constant_capacity * 2;
        chunk->constants = realloc(chunk->constants, sizeof(value_t) * new_capacity);
        chunk->constant_capacity = new_capacity;
    }
    
    
    chunk->constants[chunk->constant_count] = value;
    return chunk->constant_count++;
}

void chunk_add_debug_info(bytecode_chunk* chunk, int line, int column) {
    if (chunk->debug) {
        debug_info_add_entry(chunk->debug, chunk->count, line, column);
    }
}