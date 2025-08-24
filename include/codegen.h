#ifndef BIT_CODEGEN_H
#define BIT_CODEGEN_H

#include "ast.h"
#include "vm.h"

// Bytecode chunk for storing instructions
typedef struct {
    uint8_t* code;
    size_t count;
    size_t capacity;
    bit_value* constants;
    size_t constant_count;
    size_t constant_capacity;
} bytecode_chunk;

// Code generator state
typedef struct {
    bytecode_chunk* chunk;
    int had_error;
} codegen_t;

// Bytecode chunk functions
bytecode_chunk* chunk_create(void);
void chunk_destroy(bytecode_chunk* chunk);
void chunk_write_byte(bytecode_chunk* chunk, uint8_t byte);
void chunk_write_opcode(bytecode_chunk* chunk, opcode op);
void chunk_write_operand(bytecode_chunk* chunk, uint16_t operand);
size_t chunk_add_constant(bytecode_chunk* chunk, bit_value value);

// Code generation functions
codegen_t* codegen_create(void);
void codegen_destroy(codegen_t* codegen);

bit_function* codegen_compile(codegen_t* codegen, ast_program* program);
void codegen_emit_expression(codegen_t* codegen, ast_node* expr);
void codegen_emit_statement(codegen_t* codegen, ast_node* stmt);

// Expression code generation
void codegen_emit_number(codegen_t* codegen, ast_number* node);
void codegen_emit_string(codegen_t* codegen, ast_string* node);
void codegen_emit_boolean(codegen_t* codegen, ast_boolean* node);
void codegen_emit_null(codegen_t* codegen, ast_null* node);
void codegen_emit_undefined(codegen_t* codegen, ast_undefined* node);
void codegen_emit_identifier(codegen_t* codegen, ast_identifier* node);
void codegen_emit_binary_op(codegen_t* codegen, ast_binary_op* node);
void codegen_emit_unary_op(codegen_t* codegen, ast_unary_op* node);
void codegen_emit_array(codegen_t* codegen, ast_array* node);
void codegen_emit_object(codegen_t* codegen, ast_object_literal* node);

// Statement code generation
void codegen_emit_var_declaration(codegen_t* codegen, ast_var_declaration* node);
void codegen_emit_expression_stmt(codegen_t* codegen, ast_expression_stmt* node);
void codegen_emit_block(codegen_t* codegen, ast_block* node);
void codegen_emit_if(codegen_t* codegen, ast_if* node);
void codegen_emit_while(codegen_t* codegen, ast_while* node);
void codegen_emit_return(codegen_t* codegen, ast_return* node);

// Utility functions
void codegen_emit_op(codegen_t* codegen, opcode op);
void codegen_emit_op_operand(codegen_t* codegen, opcode op, uint16_t operand);
size_t codegen_emit_jump(codegen_t* codegen, opcode op);
void codegen_patch_jump(codegen_t* codegen, size_t offset);
void codegen_emit_loop(codegen_t* codegen, size_t loop_start);

// Error handling
void codegen_error(codegen_t* codegen, const char* message);

// Debug functions
void chunk_disassemble(bytecode_chunk* chunk, const char* name);
size_t disassemble_instruction(bytecode_chunk* chunk, size_t offset);

#endif // BIT_CODEGEN_H