#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

// Debug info functions
debug_info* debug_info_create(const char* source_code) {
    debug_info* debug = malloc(sizeof(debug_info));
    if (!debug) return NULL;
    
    debug->entries = NULL;
    debug->count = 0;
    debug->capacity = 0;
    debug->source_code = source_code; // Store reference (not owned)
    
    return debug;
}

void debug_info_destroy(debug_info* debug) {
    if (!debug) return;
    
    free(debug->entries);
    free(debug);
}

void debug_info_add_entry(debug_info* debug, size_t bytecode_offset, int line, int column) {
    if (!debug) return;
    
    // Grow array if needed
    if (debug->count >= debug->capacity) {
        size_t new_capacity = debug->capacity == 0 ? 8 : debug->capacity * 2;
        debug_info_entry* new_entries = realloc(debug->entries, sizeof(debug_info_entry) * new_capacity);
        if (!new_entries) return; // Out of memory
        
        debug->entries = new_entries;
        debug->capacity = new_capacity;
    }
    
    debug->entries[debug->count].bytecode_offset = bytecode_offset;
    debug->entries[debug->count].line = line;
    debug->entries[debug->count].column = column;
    debug->count++;
}

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

// Code generator functions
codegen_t* codegen_create(void) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create();
    codegen->had_error = 0;
    codegen->debug_mode = 0; // No debug info by default
    
    return codegen;
}

codegen_t* codegen_create_with_debug(const char* source_code) {
    codegen_t* codegen = malloc(sizeof(codegen_t));
    if (!codegen) return NULL;
    
    codegen->chunk = chunk_create_with_debug(source_code);
    if (!codegen->chunk) {
        free(codegen);
        return NULL;
    }
    
    codegen->had_error = 0;
    codegen->debug_mode = 1; // Enable debug info
    
    return codegen;
}

void codegen_destroy(codegen_t* codegen) {
    if (!codegen) return;
    
    chunk_destroy(codegen->chunk);
    free(codegen);
}

// Main compilation function
function_t* codegen_compile(codegen_t* codegen, ast_program* program) {
    if (!codegen || !program) return NULL;
    
    // Generate code for all statements
    for (size_t i = 0; i < program->statement_count; i++) {
        codegen_emit_statement(codegen, program->statements[i]);
        if (codegen->had_error) return NULL;
    }
    
    // Emit halt instruction
    codegen_emit_op(codegen, OP_HALT);
    
    // Create function from chunk
    function_t* function = function_create("main");
    if (!function) return NULL;
    
    // Transfer ownership of bytecode and constants
    function->bytecode = codegen->chunk->code;
    function->bytecode_length = codegen->chunk->count;
    function->constants = codegen->chunk->constants;
    function->constant_count = codegen->chunk->constant_count;
    function->debug = codegen->chunk->debug; // Transfer debug info
    
    // Clear chunk so it won't be freed
    codegen->chunk->code = NULL;
    codegen->chunk->constants = NULL;
    codegen->chunk->debug = NULL; // Transfer ownership of debug info
    codegen->chunk->count = 0;
    codegen->chunk->constant_count = 0;
    
    return function;
}

// Expression code generation
void codegen_emit_expression(codegen_t* codegen, ast_node* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_NUMBER:
            codegen_emit_number(codegen, (ast_number*)expr);
            break;
            
        case AST_STRING:
            codegen_emit_string(codegen, (ast_string*)expr);
            break;
            
        case AST_BOOLEAN:
            codegen_emit_boolean(codegen, (ast_boolean*)expr);
            break;
            
        case AST_NULL:
            codegen_emit_null(codegen, (ast_null*)expr);
            break;
            
        case AST_UNDEFINED:
            codegen_emit_undefined(codegen, (ast_undefined*)expr);
            break;
            
        case AST_IDENTIFIER:
            codegen_emit_identifier(codegen, (ast_identifier*)expr);
            break;
            
        case AST_BINARY_OP:
            codegen_emit_binary_op(codegen, (ast_binary_op*)expr);
            break;
            
        case AST_UNARY_OP:
            codegen_emit_unary_op(codegen, (ast_unary_op*)expr);
            break;
            
        case AST_ARRAY:
            codegen_emit_array(codegen, (ast_array*)expr);
            break;
            
        case AST_OBJECT_LITERAL:
            codegen_emit_object(codegen, (ast_object_literal*)expr);
            break;
            
        case AST_INDEX: {
            ast_index* index_node = (ast_index*)expr;
            // Generate object expression
            codegen_emit_expression(codegen, index_node->object);
            // Generate index expression
            codegen_emit_expression(codegen, index_node->index);
            // Emit index operation
            codegen_emit_op(codegen, OP_GET_INDEX);
            break;
        }
        
        case AST_MEMBER: {
            ast_member* member_node = (ast_member*)expr;
            // Generate object expression
            codegen_emit_expression(codegen, member_node->object);
            // Generate property name as string constant
            size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member_node->property));
            codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
            // Emit property access operation
            codegen_emit_op(codegen, OP_GET_PROPERTY);
            break;
        }
        
        case AST_CALL: {
            ast_call* call_node = (ast_call*)expr;
            // Generate function/callable expression first
            codegen_emit_expression(codegen, call_node->function);
            // Generate arguments (pushed left to right)
            for (size_t i = 0; i < call_node->arg_count; i++) {
                codegen_emit_expression(codegen, call_node->arguments[i]);
            }
            // Emit call operation with argument count
            codegen_emit_op_operand(codegen, OP_CALL, (uint16_t)call_node->arg_count);
            break;
        }
            
        default:
            codegen_error(codegen, "Unknown expression type");
            break;
    }
}

// Statement code generation
void codegen_emit_statement(codegen_t* codegen, ast_node* stmt) {
    if (!stmt) return;
    
    switch (stmt->type) {
        case AST_VAR_DECLARATION:
            codegen_emit_var_declaration(codegen, (ast_var_declaration*)stmt);
            break;
            
        case AST_EXPRESSION_STMT:
            codegen_emit_expression_stmt(codegen, (ast_expression_stmt*)stmt);
            break;
            
        case AST_BLOCK:
            codegen_emit_block(codegen, (ast_block*)stmt);
            break;
            
        case AST_IF:
            codegen_emit_if(codegen, (ast_if*)stmt);
            break;
            
        case AST_WHILE:
            codegen_emit_while(codegen, (ast_while*)stmt);
            break;
            
        case AST_RETURN:
            codegen_emit_return(codegen, (ast_return*)stmt);
            break;
            
        default:
            codegen_error(codegen, "Unknown statement type");
            break;
    }
}

// Literal emission functions
void codegen_emit_number(codegen_t* codegen, ast_number* node) {
    size_t constant = chunk_add_constant(codegen->chunk, make_number(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_string(codegen_t* codegen, ast_string* node) {
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->value));
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)constant);
}

void codegen_emit_boolean(codegen_t* codegen, ast_boolean* node) {
    if (node->value) {
        codegen_emit_op(codegen, OP_PUSH_TRUE);
    } else {
        codegen_emit_op(codegen, OP_PUSH_FALSE);
    }
}

void codegen_emit_null(codegen_t* codegen, ast_null* node) {
    (void)node; // Unused parameter
    codegen_emit_op(codegen, OP_PUSH_NULL);
}

void codegen_emit_undefined(codegen_t* codegen, ast_undefined* node) {
    (void)node; // Unused parameter
    codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
}

void codegen_emit_identifier(codegen_t* codegen, ast_identifier* node) {
    // For now, treat all identifiers as globals
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
    codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
}

void codegen_emit_binary_op(codegen_t* codegen, ast_binary_op* node) {
    // Generate left operand
    codegen_emit_expression(codegen, node->left);
    
    // Generate right operand  
    codegen_emit_expression(codegen, node->right);
    
    // Generate operator with debug info
    switch (node->op) {
        case BIN_ADD:           codegen_emit_op_with_debug(codegen, OP_ADD, (ast_node*)node); break;
        case BIN_SUBTRACT:      codegen_emit_op_with_debug(codegen, OP_SUBTRACT, (ast_node*)node); break;
        case BIN_MULTIPLY:      codegen_emit_op_with_debug(codegen, OP_MULTIPLY, (ast_node*)node); break;
        case BIN_DIVIDE:        codegen_emit_op_with_debug(codegen, OP_DIVIDE, (ast_node*)node); break;
        case BIN_EQUAL:         codegen_emit_op_with_debug(codegen, OP_EQUAL, (ast_node*)node); break;
        case BIN_NOT_EQUAL:     codegen_emit_op_with_debug(codegen, OP_NOT_EQUAL, (ast_node*)node); break;
        case BIN_LESS:          codegen_emit_op_with_debug(codegen, OP_LESS, (ast_node*)node); break;
        case BIN_LESS_EQUAL:    codegen_emit_op_with_debug(codegen, OP_LESS_EQUAL, (ast_node*)node); break;
        case BIN_GREATER:       codegen_emit_op_with_debug(codegen, OP_GREATER, (ast_node*)node); break;
        case BIN_GREATER_EQUAL: codegen_emit_op_with_debug(codegen, OP_GREATER_EQUAL, (ast_node*)node); break;
        case BIN_LOGICAL_AND:   codegen_emit_op_with_debug(codegen, OP_AND, (ast_node*)node); break;
        case BIN_LOGICAL_OR:    codegen_emit_op_with_debug(codegen, OP_OR, (ast_node*)node); break;
    }
}

void codegen_emit_unary_op(codegen_t* codegen, ast_unary_op* node) {
    // Generate operand
    codegen_emit_expression(codegen, node->operand);
    
    // Generate operator
    switch (node->op) {
        case UN_NEGATE: codegen_emit_op(codegen, OP_NEGATE); break;
        case UN_NOT:    codegen_emit_op(codegen, OP_NOT); break;
    }
}

void codegen_emit_array(codegen_t* codegen, ast_array* node) {
    // Generate all elements
    for (size_t i = 0; i < node->count; i++) {
        codegen_emit_expression(codegen, node->elements[i]);
    }
    
    // Build array from stack
    codegen_emit_op_operand(codegen, OP_BUILD_ARRAY, (uint16_t)node->count);
}

void codegen_emit_object(codegen_t* codegen, ast_object_literal* node) {
    // Generate key-value pairs
    for (size_t i = 0; i < node->property_count; i++) {
        // Push key
        size_t key_constant = chunk_add_constant(codegen->chunk, make_string(node->properties[i].key));
        codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)key_constant);
        
        // Push value
        codegen_emit_expression(codegen, node->properties[i].value);
    }
    
    // Build object from stack
    codegen_emit_op_operand(codegen, OP_BUILD_OBJECT, (uint16_t)node->property_count);
}

// Statement emission functions
void codegen_emit_var_declaration(codegen_t* codegen, ast_var_declaration* node) {
    if (node->initializer) {
        codegen_emit_expression(codegen, node->initializer);
    } else {
        codegen_emit_op(codegen, OP_PUSH_NULL);
    }
    
    // Define global variable
    size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
    codegen_emit_op_operand(codegen, OP_DEFINE_GLOBAL, (uint16_t)constant);
}

void codegen_emit_expression_stmt(codegen_t* codegen, ast_expression_stmt* node) {
    codegen_emit_expression(codegen, node->expression);
    codegen_emit_op(codegen, OP_POP); // Pop the result
}

void codegen_emit_block(codegen_t* codegen, ast_block* node) {
    for (size_t i = 0; i < node->statement_count; i++) {
        codegen_emit_statement(codegen, node->statements[i]);
    }
}

void codegen_emit_if(codegen_t* codegen, ast_if* node) {
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false
    size_t else_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    codegen_emit_op(codegen, OP_POP); // Pop condition
    
    // Generate then branch
    codegen_emit_statement(codegen, node->then_stmt);
    
    size_t end_jump = codegen_emit_jump(codegen, OP_JUMP);
    
    // Patch else jump
    codegen_patch_jump(codegen, else_jump);
    codegen_emit_op(codegen, OP_POP); // Pop condition
    
    // Generate else branch if present
    if (node->else_stmt) {
        codegen_emit_statement(codegen, node->else_stmt);
    }
    
    // Patch end jump
    codegen_patch_jump(codegen, end_jump);
}

void codegen_emit_while(codegen_t* codegen, ast_while* node) {
    size_t loop_start = codegen->chunk->count;
    
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false (exit loop)
    size_t exit_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    codegen_emit_op(codegen, OP_POP); // Pop condition
    
    // Generate body
    codegen_emit_statement(codegen, node->body);
    
    // Loop back
    codegen_emit_loop(codegen, loop_start);
    
    // Patch exit jump
    codegen_patch_jump(codegen, exit_jump);
    codegen_emit_op(codegen, OP_POP); // Pop condition
}

void codegen_emit_return(codegen_t* codegen, ast_return* node) {
    if (node->value) {
        codegen_emit_expression(codegen, node->value);
    } else {
        codegen_emit_op(codegen, OP_PUSH_NULL);
    }
    
    codegen_emit_op(codegen, OP_RETURN);
}

// Utility functions
void codegen_emit_op(codegen_t* codegen, opcode op) {
    chunk_write_opcode(codegen->chunk, op);
}

void codegen_emit_op_operand(codegen_t* codegen, opcode op, uint16_t operand) {
    chunk_write_opcode(codegen->chunk, op);
    chunk_write_operand(codegen->chunk, operand);
}

// Enhanced versions that add debug info from AST nodes
void codegen_emit_op_with_debug(codegen_t* codegen, opcode op, ast_node* node) {
    if (codegen->debug_mode && node) {
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

// Error handling
void codegen_error(codegen_t* codegen, const char* message) {
    fprintf(stderr, "Codegen error: %s\n", message);
    codegen->had_error = 1;
}

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
            printf("%-16s %4d '", opcode_name(instruction), constant);
            print_value(chunk->constants[constant]);
            printf("'\n");
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
        
        default:
            printf("%s\n", opcode_name(instruction));
            return offset + 1;
    }
}