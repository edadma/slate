#ifndef BIT_CODEGEN_H
#define BIT_CODEGEN_H

#include "ast.h"
#include "vm.h"

// Debug info entry mapping bytecode offset to source position
typedef struct {
    size_t bytecode_offset;
    int line;
    int column;
} debug_info_entry;

// Debug information for bytecode
typedef struct {
    debug_info_entry* entries;
    size_t count;
    size_t capacity;
    const char* source_code; // Reference to original source (not owned)
} debug_info;

// Bytecode chunk for storing instructions
typedef struct {
    uint8_t* code;
    size_t count;
    size_t capacity;
    value_t* constants;
    size_t constant_count;
    size_t constant_capacity;
    debug_info* debug; // Optional debug information (NULL if disabled)
} bytecode_chunk;

// Individual loop context for nested loop support
typedef struct {
    size_t loop_start;          // Position for continue jumps
    size_t* break_jumps;        // Break jumps for this loop level
    size_t break_count;
    size_t break_capacity;
} loop_context_t;

// Code generator state
typedef struct {
    bytecode_chunk* chunk;
    slate_vm* vm;                  // VM instance for function table access
    int had_error;
    int debug_mode; // Whether to generate debug information
    // Stack-based loop context for nested loops
    loop_context_t* loop_contexts;  // Array of loop contexts (stack)
    size_t loop_depth;             // Current nesting depth (0 = no loops) 
    size_t loop_capacity;          // Capacity of loop_contexts array
} codegen_t;

// Debug info functions
debug_info* debug_info_create(const char* source_code);
void debug_info_destroy(debug_info* debug);
void debug_info_add_entry(debug_info* debug, size_t bytecode_offset, int line, int column);

// Bytecode chunk functions
bytecode_chunk* chunk_create(void);
bytecode_chunk* chunk_create_with_debug(const char* source_code);
void chunk_destroy(bytecode_chunk* chunk);
void chunk_write_byte(bytecode_chunk* chunk, uint8_t byte);
void chunk_write_opcode(bytecode_chunk* chunk, opcode op);
void chunk_write_operand(bytecode_chunk* chunk, uint16_t operand);
size_t chunk_add_constant(bytecode_chunk* chunk, value_t value);
void chunk_add_debug_info(bytecode_chunk* chunk, int line, int column);

// Code generation functions
codegen_t* codegen_create(slate_vm* vm);
codegen_t* codegen_create_with_debug(slate_vm* vm, const char* source_code);
void codegen_destroy(codegen_t* codegen);

function_t* codegen_compile(codegen_t* codegen, ast_program* program);
void codegen_emit_expression(codegen_t* codegen, ast_node* expr);
void codegen_emit_statement(codegen_t* codegen, ast_node* stmt);

// Expression code generation
void codegen_emit_integer(codegen_t* codegen, ast_integer* node);
void codegen_emit_bigint(codegen_t* codegen, ast_bigint* node);
void codegen_emit_number(codegen_t* codegen, ast_number* node);
void codegen_emit_string(codegen_t* codegen, ast_string* node);
void codegen_emit_boolean(codegen_t* codegen, ast_boolean* node);
void codegen_emit_null(codegen_t* codegen, ast_null* node);
void codegen_emit_undefined(codegen_t* codegen, ast_undefined* node);
void codegen_emit_identifier(codegen_t* codegen, ast_identifier* node);
void codegen_emit_binary_op(codegen_t* codegen, ast_binary_op* node);
void codegen_emit_range(codegen_t* codegen, ast_range* node);
void codegen_emit_unary_op(codegen_t* codegen, ast_unary_op* node);
void codegen_emit_array(codegen_t* codegen, ast_array* node);
void codegen_emit_object(codegen_t* codegen, ast_object_literal* node);
// Function codegen will be added later with proper design

// Statement code generation
void codegen_emit_var_declaration(codegen_t* codegen, ast_var_declaration* node);
void codegen_emit_assignment(codegen_t* codegen, ast_assignment* node);
void codegen_emit_compound_assignment(codegen_t* codegen, ast_compound_assignment* node);
void codegen_emit_expression_stmt(codegen_t* codegen, ast_expression_stmt* node);
void codegen_emit_block(codegen_t* codegen, ast_block* node);
void codegen_emit_block_expression(codegen_t* codegen, ast_block* node);
void codegen_emit_if(codegen_t* codegen, ast_if* node);
void codegen_emit_while(codegen_t* codegen, ast_while* node);
void codegen_emit_infinite_loop(codegen_t* codegen, ast_loop* node);
void codegen_emit_break(codegen_t* codegen, ast_break* node);
void codegen_emit_continue(codegen_t* codegen, ast_continue* node);
void codegen_emit_return(codegen_t* codegen, ast_return* node);

// Utility functions
void codegen_emit_op(codegen_t* codegen, opcode op);
void codegen_emit_op_operand(codegen_t* codegen, opcode op, uint16_t operand);
void codegen_emit_debug_location(codegen_t* codegen, ast_node* node);
void codegen_emit_op_with_debug(codegen_t* codegen, opcode op, ast_node* node);
void codegen_emit_op_operand_with_debug(codegen_t* codegen, opcode op, uint16_t operand, ast_node* node);
size_t codegen_emit_jump(codegen_t* codegen, opcode op);
void codegen_patch_jump(codegen_t* codegen, size_t offset);
void codegen_emit_loop(codegen_t* codegen, size_t loop_start);

// Loop management for break and continue statements (nested support)
void codegen_push_loop(codegen_t* codegen, size_t loop_start);
void codegen_pop_loop(codegen_t* codegen);
loop_context_t* codegen_current_loop(codegen_t* codegen);

// Error handling
void codegen_error(codegen_t* codegen, const char* message);

// Debug functions
void chunk_disassemble(bytecode_chunk* chunk, const char* name);
size_t disassemble_instruction(bytecode_chunk* chunk, size_t offset);

#endif // BIT_CODEGEN_H