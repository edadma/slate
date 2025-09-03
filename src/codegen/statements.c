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
    if (node->target->type == AST_IDENTIFIER) {
        // Variable assignment: var = value
        // Generate the value first
        codegen_emit_expression(codegen, node->value);
        
        ast_identifier* var = (ast_identifier*)node->target;
        codegen_emit_op(codegen, OP_DUP);
        
        // Try 3-level resolution: local -> upvalue -> global
        int is_local;
        int upvalue_index;
        int slot = codegen_resolve_variable(codegen, var->name, &is_local, &upvalue_index);
        
        if (is_local) {
            // Local variable assignment with single byte operand
            codegen_emit_op(codegen, OP_SET_LOCAL);
            chunk_write_byte(codegen->chunk, (uint8_t)slot);
        } else if (upvalue_index != -1) {
            // Upvalue assignment with single byte operand
            codegen_emit_op(codegen, OP_SET_UPVALUE);
            chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
        } else {
            // Global variable assignment
            size_t constant = chunk_add_constant(codegen->chunk, make_string(var->name));
            
            // Add debug info for the assignment operation
            chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
            codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
        }
        
    } else if (node->target->type == AST_MEMBER) {
        // Object property assignment: obj.prop = value
        ast_member* member = (ast_member*)node->target;
        
        // Generate in order: object, property_name, value
        codegen_emit_expression(codegen, member->object);
        
        size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member->property));
        codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
        
        codegen_emit_expression(codegen, node->value);
        
        // Stack: [object, property_name, value] - matches OP_SET_PROPERTY
        // OP_SET_PROPERTY will push the assigned value back, then we need to pop it for statements
        codegen_emit_op(codegen, OP_SET_PROPERTY);
        codegen_emit_op(codegen, OP_POP);  // Discard the returned value in statement context
        
    } else if (node->target->type == AST_CALL) {
        // Array element assignment: arr(index) = value
        ast_call* call = (ast_call*)node->target;
        
        // Validate that this is array indexing (exactly one argument)
        if (call->arg_count != 1) {
            codegen_error(codegen, "Array assignment requires exactly one index argument");
            return;
        }
        
        // Generate in order: array, index, value
        codegen_emit_expression(codegen, call->function);
        codegen_emit_expression(codegen, call->arguments[0]);
        codegen_emit_expression(codegen, node->value);
        
        // Stack: [array, index, value] - matches OP_SET_INDEX
        // OP_SET_INDEX will push the assigned value back, then we need to pop it for statements  
        codegen_emit_op(codegen, OP_SET_INDEX);
        codegen_emit_op(codegen, OP_POP);  // Discard the returned value in statement context
        
    } else {
        codegen_error(codegen, "Invalid assignment target");
    }
}

void codegen_emit_compound_assignment(codegen_t* codegen, ast_compound_assignment* node) {
    // For now, only handle simple variable compound assignments
    if (node->target->type != AST_IDENTIFIER) {
        codegen_error(codegen, "Only variable compound assignments are currently supported");
        return;
    }
    
    ast_identifier* var = (ast_identifier*)node->target;
    
    // Try 3-level resolution: local -> upvalue -> global
    int is_local;
    int upvalue_index;
    int slot = codegen_resolve_variable(codegen, var->name, &is_local, &upvalue_index);
    
    // Get the current value of the variable
    chunk_add_debug_info(codegen->chunk, node->base.line, node->base.column);
    if (is_local) {
        codegen_emit_op(codegen, OP_GET_LOCAL);
        chunk_write_byte(codegen->chunk, (uint8_t)slot);
    } else if (upvalue_index != -1) {
        codegen_emit_op(codegen, OP_GET_UPVALUE);
        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
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
    } else if (upvalue_index != -1) {
        codegen_emit_op(codegen, OP_SET_UPVALUE);
        chunk_write_byte(codegen->chunk, (uint8_t)upvalue_index);
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

void codegen_emit_import(codegen_t* codegen, ast_import* node) {
    // For now, import statements are processed at compile time and don't generate runtime code
    // The actual module loading and symbol resolution will happen during VM execution
    
    // Add the module path as a constant
    size_t module_path_constant = chunk_add_constant(codegen->chunk, make_string(node->module_path));
    
    if (node->is_wildcard) {
        // Wildcard import: import module._
        // This imports all exports from the module into the current namespace
        codegen_emit_op_operand(codegen, OP_IMPORT_MODULE, (uint16_t)module_path_constant);
        // The wildcard flag is encoded as 0xFF in the next byte
        chunk_write_byte(codegen->chunk, 0xFF);
        chunk_write_byte(codegen->chunk, 0); // No specific specifiers for wildcard
    } else if (node->specifier_count > 0) {
        // Specific imports: import module.{name1, name2 => alias}
        codegen_emit_op_operand(codegen, OP_IMPORT_MODULE, (uint16_t)module_path_constant);
        
        // Write the number of specifiers
        chunk_write_byte(codegen->chunk, (uint8_t)node->specifier_count);
        
        // For each specifier, add the name and optional alias as constants
        for (size_t i = 0; i < node->specifier_count; i++) {
            import_specifier* spec = &node->specifiers[i];
            
            // Add the original name as a constant
            size_t name_constant = chunk_add_constant(codegen->chunk, make_string(spec->name));
            chunk_write_byte(codegen->chunk, (uint8_t)name_constant);
            
            // Add the alias (or use the same name if no alias)
            const char* local_name = spec->alias ? spec->alias : spec->name;
            size_t alias_constant = chunk_add_constant(codegen->chunk, make_string(local_name));
            chunk_write_byte(codegen->chunk, (uint8_t)alias_constant);
        }
    } else {
        // Namespace import: import module (creates namespace object)
        codegen_emit_op_operand(codegen, OP_IMPORT_MODULE, (uint16_t)module_path_constant);
        // The namespace flag is encoded as 0xFE in the next byte
        chunk_write_byte(codegen->chunk, 0xFE);
        
        // Extract the last part of the module path as the namespace name
        // e.g., "examples.modules.utils" -> "utils"
        const char* last_dot = strrchr(node->module_path, '.');
        const char* namespace_name = last_dot ? (last_dot + 1) : node->module_path;
        
        // Add namespace name as constant
        size_t namespace_constant = chunk_add_constant(codegen->chunk, make_string(namespace_name));
        chunk_write_byte(codegen->chunk, (uint8_t)namespace_constant);
    }
    
    // Import statements don't produce a value, but we need something for the result register
    codegen_emit_op(codegen, OP_PUSH_NULL);
    codegen_emit_op(codegen, OP_SET_RESULT);
}

void codegen_emit_package(codegen_t* codegen, ast_package* node) {
    // Package declarations are mainly metadata and don't generate runtime code
    // They could be used to validate that the file is in the correct package structure
    
    // For now, we'll just emit a comment-like no-op
    // In a more sophisticated implementation, this could:
    // 1. Validate the package name matches the file path
    // 2. Set up module exports namespace
    // 3. Register the module with the package system
    
    // Add the package name as a constant for potential debugging/reflection use
    size_t package_constant = chunk_add_constant(codegen->chunk, make_string(node->package_name));
    
    // This is essentially a no-op at runtime, but we store the package info
    // Future enhancement: could emit OP_SET_PACKAGE_INFO or similar
    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)package_constant);
    codegen_emit_op(codegen, OP_POP); // Discard the package name
    
    // Package statements don't produce a value
    codegen_emit_op(codegen, OP_PUSH_NULL);
    codegen_emit_op(codegen, OP_SET_RESULT);
}