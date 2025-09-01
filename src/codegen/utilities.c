#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Utility functions
void codegen_emit_op(codegen_t* codegen, opcode op) {
    chunk_write_opcode(codegen->chunk, op);
}

void codegen_emit_op_operand(codegen_t* codegen, opcode op, uint16_t operand) {
    chunk_write_opcode(codegen->chunk, op);
    chunk_write_operand(codegen->chunk, operand);
}

// Helper function to get a specific line from source code
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
    
    // If we didn't find the line, return NULL
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

// Enhanced versions that add debug info from AST nodes
// Helper function to emit debug location instruction
void codegen_emit_debug_location(codegen_t* codegen, ast_node* node) {
    if (!codegen->debug_mode || !node || !codegen->chunk->debug || !codegen->chunk->debug->source_code) {
        return;
    }
    
    // Get the source line text
    size_t line_length;
    const char* line_start = get_source_line(codegen->chunk->debug->source_code, node->line, &line_length);
    if (!line_start) return;
    
    // Create a null-terminated copy of the line for storage as a string constant
    char* line_copy = malloc(line_length + 1);
    if (!line_copy) return;
    strncpy(line_copy, line_start, line_length);
    line_copy[line_length] = '\0';
    
    // Store the source line text as a string constant
    size_t constant_index = chunk_add_constant(codegen->chunk, make_string(line_copy));
    
    // For now, we'll store line and column in the operand (16 bits total)
    // Upper 8 bits = line number (limited to 255), lower 8 bits = column (limited to 255) 
    uint16_t location_info = ((node->line & 0xFF) << 8) | (node->column & 0xFF);
    
    // Emit the debug location instruction with the constant index
    codegen_emit_op_operand(codegen, OP_SET_DEBUG_LOCATION, (uint16_t)constant_index);
    
    // Follow with a special instruction that carries the line/column info
    chunk_write_byte(codegen->chunk, (location_info >> 8) & 0xFF); // line
    chunk_write_byte(codegen->chunk, location_info & 0xFF); // column
    
    free(line_copy);
}

void codegen_emit_op_with_debug(codegen_t* codegen, opcode op, ast_node* node) {
    if (codegen->debug_mode && node) {
        // Emit debug location instruction before the actual operation
        codegen_emit_debug_location(codegen, node);
        chunk_add_debug_info(codegen->chunk, node->line, node->column);
    }
    chunk_write_opcode(codegen->chunk, op);
}

void codegen_emit_op_operand_with_debug(codegen_t* codegen, opcode op, uint16_t operand, ast_node* node) {
    if (codegen->debug_mode && node) {
        chunk_add_debug_info(codegen->chunk, node->line, node->column);
    }
    chunk_write_opcode(codegen->chunk, op);
    chunk_write_operand(codegen->chunk, operand);
}

size_t codegen_emit_jump(codegen_t* codegen, opcode op) {
    codegen_emit_op_operand(codegen, op, 0xFFFF); // Placeholder
    return codegen->chunk->count - 2; // Return offset of operand
}

void codegen_patch_jump(codegen_t* codegen, size_t offset) {
    // Calculate jump distance
    size_t jump = codegen->chunk->count - offset - 2;
    
    if (jump > UINT16_MAX) {
        codegen_error(codegen, "Too much code to jump over");
        return;
    }
    
    // Patch the jump operand
    codegen->chunk->code[offset] = (uint8_t)(jump & 0xFF);
    codegen->chunk->code[offset + 1] = (uint8_t)((jump >> 8) & 0xFF);
}

void codegen_emit_loop(codegen_t* codegen, size_t loop_start) {
    size_t offset = codegen->chunk->count - loop_start + 3; // +3 for the LOOP instruction itself
    
    if (offset > UINT16_MAX) {
        codegen_error(codegen, "Loop body too large");
        return;
    }
    
    codegen_emit_op_operand(codegen, OP_LOOP, (uint16_t)offset);
}