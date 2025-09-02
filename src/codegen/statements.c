#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Statement emission functions
void codegen_emit_var_declaration(codegen_t* codegen, ast_var_declaration* node) {
    if (codegen->scope.scope_depth == 0) {
        // Global variable declaration
        if (node->initializer) {
            codegen_emit_expression(codegen, node->initializer);
        } else {
            codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
        }
        
        // Duplicate the value so we can store it and also set result
        codegen_emit_op(codegen, OP_DUP);
        
        // Define global variable
        size_t constant = chunk_add_constant(codegen->chunk, make_string(node->name));
        codegen_emit_op_operand(codegen, OP_DEFINE_GLOBAL, (uint16_t)constant);
        
        // Emit immutability flag (1 byte)
        chunk_write_byte(codegen->chunk, node->is_immutable ? 1 : 0);
        
        // Set result register with the initialization value
        codegen_emit_op(codegen, OP_SET_RESULT);
    } else {
        // Local variable declaration - modify declare function to allow re-initialization
        int slot = codegen_declare_variable(codegen, node->name, node->is_immutable);
        if (slot < 0) {
            return; // Error already reported
        }
        
        if (node->initializer) {
            codegen_emit_expression(codegen, node->initializer);
        } else {
            codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
        }
        
        // Always use SET_LOCAL approach for consistency
        codegen_emit_op(codegen, OP_DUP);
        codegen_emit_op(codegen, OP_SET_LOCAL);
        chunk_write_byte(codegen->chunk, (uint8_t)slot);
        
        // Set result register with the initialization value
        codegen_emit_op(codegen, OP_SET_RESULT);
    }
}

void codegen_emit_assignment(codegen_t* codegen, ast_assignment* node) {
    // Generate code for the value to assign
    codegen_emit_expression(codegen, node->value);
    
    // For now, only handle simple variable assignments
    if (node->target->type == AST_IDENTIFIER) {
        ast_identifier* var = (ast_identifier*)node->target;
        codegen_emit_op(codegen, OP_DUP);
        
        // Try to resolve as local variable first
        int is_local;
        int slot = codegen_resolve_variable(codegen, var->name, &is_local);
        
        if (is_local) {
            // Local variable assignment with single byte operand
            codegen_emit_op(codegen, OP_SET_LOCAL);
            chunk_write_byte(codegen->chunk, (uint8_t)slot);
        } else {
            // Global variable assignment
            size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
            
            // Add debug info for the assignment operation
            chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
            codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
        }
    } else {
        codegen_error(codegen, "Only variable assignments are currently supported");
    }
}

void codegen_emit_compound_assignment(codegen_t* codegen, ast_compound_assignment* node) {
    // For now, only handle simple variable compound assignments
    if (node->target->type != AST_IDENTIFIER) {
        codegen_error(codegen, "Only variable compound assignments are currently supported");
        return;
    }
    
    ast_identifier* var = (ast_identifier*)node->target;
    
    // Try to resolve as local variable first
    int is_local;
    int slot = codegen_resolve_variable(codegen, var->name, &is_local);
    
    // Get the current value of the variable
    chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
    if (is_local) {
        codegen_emit_op(codegen, OP_GET_LOCAL);
        chunk_write_byte(codegen->chunk, (uint8_t)slot);
    } else {
        size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
        codegen_emit_op_operand(codegen, OP_GET_GLOBAL, (uint16_t)constant);
    }
    
    // Generate code for the right-hand side value
    codegen_emit_expression(codegen, node->value);
    
    // Emit the appropriate binary operation
    switch (node->op) {
        case BIN_ADD:      codegen_emit_op(codegen, OP_ADD); break;
        case BIN_SUBTRACT: codegen_emit_op(codegen, OP_SUBTRACT); break;
        case BIN_MULTIPLY: codegen_emit_op(codegen, OP_MULTIPLY); break;
        case BIN_DIVIDE:   codegen_emit_op(codegen, OP_DIVIDE); break;
        case BIN_MOD:      codegen_emit_op(codegen, OP_MOD); break;
        case BIN_POWER:    codegen_emit_op(codegen, OP_POWER); break;
        case BIN_BITWISE_AND:  codegen_emit_op(codegen, OP_BITWISE_AND); break;
        case BIN_BITWISE_OR:   codegen_emit_op(codegen, OP_BITWISE_OR); break;
        case BIN_BITWISE_XOR:  codegen_emit_op(codegen, OP_BITWISE_XOR); break;
        case BIN_LEFT_SHIFT:   codegen_emit_op(codegen, OP_LEFT_SHIFT); break;
        case BIN_RIGHT_SHIFT:  codegen_emit_op(codegen, OP_RIGHT_SHIFT); break;
        case BIN_LOGICAL_RIGHT_SHIFT: codegen_emit_op(codegen, OP_LOGICAL_RIGHT_SHIFT); break;
        case BIN_LOGICAL_AND:  codegen_emit_op(codegen, OP_AND); break;
        case BIN_LOGICAL_OR:   codegen_emit_op(codegen, OP_OR); break;
        case BIN_NULL_COALESCE: codegen_emit_op(codegen, OP_NULL_COALESCE); break;
        default:
            codegen_error(codegen, "Unsupported compound assignment operation");
            return;
    }
    
    // Duplicate the result for expression contexts
    codegen_emit_op(codegen, OP_DUP);
    
    // Store the result back to the variable
    chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
    if (is_local) {
        codegen_emit_op(codegen, OP_SET_LOCAL);
        chunk_write_byte(codegen->chunk, (uint8_t)slot);
    } else {
        size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
        codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
    }
}

void codegen_emit_expression_stmt(codegen_t* codegen, ast_expression_stmt* node) {
    codegen_emit_expression(codegen, node->expression);
    codegen_emit_op(codegen, OP_SET_RESULT); // Pop and store in result register
}

void codegen_emit_block(codegen_t* codegen, ast_block* node) {
    // Begin new scope for the block
    codegen_begin_scope(codegen);
    
    for (size_t i = 0; i < node->statement_count; i++) {
        codegen_emit_statement(codegen, node->statements[i]);
        if (codegen->had_error) break;
    }
    
    // End scope (this will emit OP_POP_N to clean up local variables)
    codegen_end_scope(codegen);
}

// Helper function to emit a node that can be either expression or statement
void codegen_emit_expression_or_statement(codegen_t* codegen, ast_node* node) {
    if (!node) {
        codegen_emit_op(codegen, OP_PUSH_NULL);
        return;
    }
    
    // Check if this is a statement that should be handled specially
    switch (node->type) {
        case AST_BLOCK:
            // For blocks, we need to handle them as block expressions to get the result
            codegen_emit_block_expression(codegen, (ast_block*)node);
            break;
        case AST_EXPRESSION_STMT:
            // For expression statements, emit the inner expression directly
            // This is important for single-line if statements: if true then 42
            codegen_emit_expression(codegen, ((ast_expression_stmt*)node)->expression);
            break;
        case AST_VAR_DECLARATION:
        case AST_ASSIGNMENT:  
        case AST_COMPOUND_ASSIGNMENT:
        case AST_WHILE:
        case AST_DO_WHILE:
        case AST_LOOP:
        case AST_BREAK:
        case AST_CONTINUE:
        case AST_RETURN:
            // These are statements - emit as statements and push null as result
            codegen_emit_statement(codegen, node);
            codegen_emit_op(codegen, OP_PUSH_NULL);
            break;
        default:
            // This is an expression - emit it normally
            codegen_emit_expression(codegen, node);
            break;
    }
}

void codegen_emit_if(codegen_t* codegen, ast_if* node) {
    // Generate condition
    codegen_emit_expression(codegen, node->condition);
    
    // Jump if false (OP_JUMP_IF_FALSE pops the condition automatically)
    size_t else_jump = codegen_emit_jump(codegen, OP_JUMP_IF_FALSE);
    
    // Generate then branch (can be expression or statement)
    // Use direct dispatch to avoid recursion with codegen_emit_expression_or_statement
    if (node->then_stmt->type == AST_BLOCK) {
        codegen_emit_block_expression(codegen, (ast_block*)node->then_stmt);
    } else if (node->then_stmt->type == AST_EXPRESSION_STMT) {
        codegen_emit_expression(codegen, ((ast_expression_stmt*)node->then_stmt)->expression);
    } else if (node->then_stmt->type == AST_VAR_DECLARATION ||
               node->then_stmt->type == AST_ASSIGNMENT ||
               node->then_stmt->type == AST_COMPOUND_ASSIGNMENT ||
               node->then_stmt->type == AST_WHILE ||
               node->then_stmt->type == AST_DO_WHILE ||
               node->then_stmt->type == AST_LOOP ||
               node->then_stmt->type == AST_BREAK ||
               node->then_stmt->type == AST_CONTINUE ||
               node->then_stmt->type == AST_RETURN) {
        codegen_emit_statement(codegen, node->then_stmt);
        codegen_emit_op(codegen, OP_PUSH_NULL);
    } else {
        // Regular expression
        codegen_emit_expression(codegen, node->then_stmt);
    }
    
    size_t end_jump = codegen_emit_jump(codegen, OP_JUMP);
    
    // Patch else jump (no need to pop condition - OP_JUMP_IF_FALSE already did)
    codegen_patch_jump(codegen, else_jump);
    
    // Generate else branch if present (can be expression or statement)
    if (node->else_stmt) {
        // Use direct dispatch to avoid recursion with codegen_emit_expression_or_statement
        if (node->else_stmt->type == AST_BLOCK) {
            codegen_emit_block_expression(codegen, (ast_block*)node->else_stmt);
        } else if (node->else_stmt->type == AST_EXPRESSION_STMT) {
            codegen_emit_expression(codegen, ((ast_expression_stmt*)node->else_stmt)->expression);
        } else if (node->else_stmt->type == AST_VAR_DECLARATION ||
                   node->else_stmt->type == AST_ASSIGNMENT ||
                   node->else_stmt->type == AST_COMPOUND_ASSIGNMENT ||
                   node->else_stmt->type == AST_WHILE ||
                   node->else_stmt->type == AST_DO_WHILE ||
                   node->else_stmt->type == AST_LOOP ||
                   node->else_stmt->type == AST_BREAK ||
                   node->else_stmt->type == AST_CONTINUE ||
                   node->else_stmt->type == AST_RETURN) {
            codegen_emit_statement(codegen, node->else_stmt);
            codegen_emit_op(codegen, OP_PUSH_NULL);
        } else {
            // Regular expression
            codegen_emit_expression(codegen, node->else_stmt);
        }
    } else {
        // If no else branch, push null as the result
        codegen_emit_op(codegen, OP_PUSH_NULL);
    }
    
    // Patch end jump
    codegen_patch_jump(codegen, end_jump);
}

void codegen_emit_block_expression(codegen_t* codegen, ast_block* node) {
    if (node->statement_count == 0) {
        // Empty block returns null
        codegen_emit_op(codegen, OP_PUSH_NULL);
        return;
    }
    
    // Begin new scope for the block expression
    codegen_begin_scope(codegen);
    
    // Execute all statements except the last one normally
    for (size_t i = 0; i < node->statement_count - 1; i++) {
        codegen_emit_statement(codegen, node->statements[i]);
        if (codegen->had_error) break;
    }
    
    if (!codegen->had_error) {
        // The last statement must be an expression statement (validated by parser)
        // Emit its expression directly to leave the value on the stack
        ast_node* last_stmt = node->statements[node->statement_count - 1];
        ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
        codegen_emit_expression(codegen, expr_stmt->expression);
    }
    
    // End scope with result preservation
    // The result is on the stack top and must be preserved during cleanup
    if (codegen->scope.scope_depth == 0) {
        codegen_error(codegen, "Cannot end global scope");
        return;
    }
    
    // Count how many locals need to be popped
    int locals_to_pop = 0;
    for (int i = codegen->scope.local_count - 1; i >= 0; i--) {
        if (codegen->scope.locals[i].depth < codegen->scope.scope_depth) {
            break; // Found variable from outer scope
        }
        locals_to_pop++;
    }
    
    // Use OP_POP_N_PRESERVE_TOP to clean up local variables while preserving the result
    if (locals_to_pop > 0) {
        codegen_emit_op_operand(codegen, OP_POP_N_PRESERVE_TOP, (uint16_t)locals_to_pop);
        codegen->scope.local_count -= locals_to_pop;
    }
    
    codegen->scope.scope_depth--;
}