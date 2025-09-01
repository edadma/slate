#include "codegen.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations
void codegen_emit_template_literal(codegen_t* codegen, ast_template_literal* node);

// Expression code generation
void codegen_emit_expression(codegen_t* codegen, ast_node* expr) {
    if (!expr) return;
    
    switch (expr->type) {
        case AST_INTEGER:
            codegen_emit_integer(codegen, (ast_integer*)expr);
            break;
            
        case AST_BIGINT:
            codegen_emit_bigint(codegen, (ast_bigint*)expr);
            break;
            
        case AST_NUMBER:
            codegen_emit_number(codegen, (ast_number*)expr);
            break;
            
        case AST_STRING:
            codegen_emit_string(codegen, (ast_string*)expr);
            break;
            
        case AST_TEMPLATE_LITERAL:
            codegen_emit_template_literal(codegen, (ast_template_literal*)expr);
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
            
        case AST_TERNARY:
            codegen_emit_ternary(codegen, (ast_ternary*)expr);
            break;
            
        case AST_RANGE:
            codegen_emit_range(codegen, (ast_range*)expr);
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
        
        case AST_FUNCTION:
            codegen_emit_function(codegen, (ast_function*)expr);
            break;
        
        case AST_ASSIGNMENT: {
            ast_assignment* assign = (ast_assignment*)expr;
            
            // Generate code for the value to assign
            codegen_emit_expression(codegen, assign->value);
            
            // For expressions, we need to duplicate the value so it remains on stack
            codegen_emit_op(codegen, OP_DUP);
            
            // Handle assignment target
            if (assign->target->type == AST_IDENTIFIER) {
                ast_identifier* var = (ast_identifier*)assign->target;
                
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
                    chunk_add_debug_info(codegen->chunk, assign->base.line, assign->base.column);
                    codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                }
            } else {
                codegen_error(codegen, "Only variable assignments are currently supported");
            }
            break;
        }
        
        case AST_COMPOUND_ASSIGNMENT: {
            ast_compound_assignment* comp_assign = (ast_compound_assignment*)expr;
            
            // For compound assignments in expressions, we emit the same logic as statements
            codegen_emit_compound_assignment(codegen, comp_assign);
            
            break;
        }

        case AST_IF:
            codegen_emit_if(codegen, (ast_if*)expr);
            break;

        case AST_BLOCK:
            codegen_emit_block_expression(codegen, (ast_block*)expr);
            break;
            
        case AST_BREAK:
            codegen_emit_break(codegen, (ast_break*)expr);
            break;
            
        case AST_CONTINUE:
            codegen_emit_continue(codegen, (ast_continue*)expr);
            break;
            
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
            
        case AST_FOR:
            codegen_emit_for(codegen, (ast_for*)stmt);
            break;
            
        case AST_DO_WHILE:
            codegen_emit_do_while(codegen, (ast_do_while*)stmt);
            break;
            
        case AST_LOOP:
            codegen_emit_infinite_loop(codegen, (ast_loop*)stmt);
            break;
            
        case AST_BREAK:
            codegen_emit_break(codegen, (ast_break*)stmt);
            break;
            
        case AST_CONTINUE:
            codegen_emit_continue(codegen, (ast_continue*)stmt);
            break;
            
        case AST_RETURN:
            codegen_emit_return(codegen, (ast_return*)stmt);
            break;
            
        case AST_ASSIGNMENT:
            codegen_emit_assignment(codegen, (ast_assignment*)stmt);
            break;
            
        case AST_COMPOUND_ASSIGNMENT:
            codegen_emit_compound_assignment(codegen, (ast_compound_assignment*)stmt);
            break;
            
        default:
            codegen_error(codegen, "Unknown statement type");
            break;
    }
}