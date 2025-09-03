#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Literal emission functions
void codegen_emit_integer(codegen_t* codegen, ast_integer* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    size_t constant = chunk_add_constant(codegen->chunk, make_int32(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_bigint(codegen_t* codegen, ast_bigint* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    // Retain the BigInt value when transferring from AST to VM
    di_int retained_value = di_retain(node->value);
    size_t constant = chunk_add_constant(codegen->chunk, make_bigint(retained_value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_number(codegen_t* codegen, ast_number* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    value_t value;
    if (node->is_float32) {
        value = make_float32(node->value.float32);
    } else {
        value = make_float64(node->value.float64);
    }
    
    size_t constant = chunk_add_constant(codegen->chunk, value);
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_string(codegen_t* codegen, ast_string* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_template_literal(codegen_t* codegen, ast_template_literal* node) {
    // Emit debug location before processing template
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    // Desugar to: StringBuilder().append(...).append(...).toString()
    
    // 1. Create a new StringBuilder: StringBuilder()
    size_t sb_constant = chunk_add_constant(codegen->chunk, make_string("StringBuilder"));
    codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)sb_constant);
    codegen_emit_op_operand(codegen, OP_CALL, 0); // Call with 0 arguments
    
    // 2. Process each part, calling append() for each
    for (size_t i = 0; i < node->part_count; i++) {
        // Stack: [StringBuilder]
        
        // Duplicate the StringBuilder (for chaining)
        codegen_emit_op(codegen, OP_DUP);
        
        // Get the append method from it
        size_t append_constant = chunk_add_constant(codegen->chunk, make_string("append"));
        codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)append_constant);
        codegen_emit_op(codegen, OP_GET_PROPERTY);
        
        // Stack: [StringBuilder, StringBuilder.append]
        
        // Push the argument for append()
        if (node->parts[i].type == TEMPLATE_PART_TEXT) {
            // Static text - push as string
            size_t text_constant = chunk_add_constant(codegen->chunk, make_string(node->parts[i].as.text));
            codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)text_constant);
        } else {
            // Expression - evaluate it
            codegen_emit_expression(codegen, node->parts[i].as.expression);
        }
        
        // Stack: [StringBuilder, StringBuilder.append, arg]
        // Call append with 1 argument (the method already has its receiver bound)
        codegen_emit_op_operand(codegen, OP_CALL, 1);
        
        // Stack: [StringBuilder, StringBuilder] (append returns the StringBuilder for chaining)
        // Pop the duplicate since append already returns the StringBuilder
        codegen_emit_op(codegen, OP_POP);
        
        // Stack: [StringBuilder] ready for next iteration
    }
    
    // 3. Finally call toString() to get the final string
    // Stack: [StringBuilder]
    // Don't duplicate - just get toString method directly
    size_t tostring_constant = chunk_add_constant(codegen->chunk, make_string("toString"));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)tostring_constant);
    codegen_emit_op(codegen, OP_GET_PROPERTY);
    
    // Stack: [StringBuilder.toString]
    codegen_emit_op_operand(codegen, OP_CALL, 0); // toString takes no arguments
    
    // Stack: [string] - the final result!
}

void codegen_emit_boolean(codegen_t* codegen, ast_boolean* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    if (node->value) {
        codegen_emit_op(codegen, OP_PUSH_TRUE);
    } else {
        codegen_emit_op(codegen, OP_PUSH_FALSE);
    }
}

void codegen_emit_null(codegen_t* codegen, ast_null* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    codegen_emit_op(codegen, OP_PUSH_NULL);
}

void codegen_emit_undefined(codegen_t* codegen, ast_undefined* node) {
    // Emit debug location before pushing the value
    codegen_emit_debug_location(codegen, (ast_node*)node);
    
    codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
}

void codegen_emit_identifier(codegen_t* codegen, ast_identifier* node) {
    // Try 3-level resolution: local -> upvalue -> global
    int is_local;
    int upvalue_index;
    int slot = codegen_resolve_variable(codegen, node->name, &is_local, &upvalue_index);
    
    if (is_local) {
        // Local variable - use OP_GET_LOCAL with single byte operand
        codegen_emit_op(codegen, OP_GET_LOCAL);
        chunk_write_byte(codegen->chunk, (uint8_t)slot);
    } else if (upvalue_index != -1) {
        // Upvalue - use OP_GET_UPVALUE with single byte operand
        codegen_emit_op(codegen, OP_GET_UPVALUE);
        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
    } else {
        // Global variable - use OP_GET_GLOBAL
        size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
    }
}