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
            
            if (member_node->is_optional) {
                // Optional chaining: obj?.prop
                // 1. Evaluate object
                codegen_emit_expression(codegen, member_node->object);
                
                // 2. Duplicate for null/undefined check
                codegen_emit_op(codegen, OP_DUP);
                
                // 3. Check if null
                codegen_emit_op(codegen, OP_PUSH_NULL);
                codegen_emit_op(codegen, OP_EQUAL);
                
                // 4. If null, jump to push undefined
                size_t null_jump = codegen_emit_jump(codegen, OP_JUMP_IF_TRUE);
                
                // 5. Check if undefined (object still on stack from DUP)
                codegen_emit_op(codegen, OP_DUP);
                codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
                codegen_emit_op(codegen, OP_EQUAL);
                
                // 6. If undefined, jump to push undefined  
                size_t undefined_jump = codegen_emit_jump(codegen, OP_JUMP_IF_TRUE);
                
                // 7. Not null/undefined: do normal property access
                // Object is already on stack
                size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member_node->property));
                codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
                codegen_emit_op(codegen, OP_GET_PROPERTY);
                
                // 8. Jump to end
                size_t end_jump = codegen_emit_jump(codegen, OP_JUMP);
                
                // 9. Patch null jump: pop object and push undefined
                codegen_patch_jump(codegen, null_jump);
                codegen_emit_op(codegen, OP_POP); // Pop the object
                codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
                
                // 10. Jump to end
                size_t null_end_jump = codegen_emit_jump(codegen, OP_JUMP);
                
                // 11. Patch undefined jump: pop object and push undefined
                codegen_patch_jump(codegen, undefined_jump);
                codegen_emit_op(codegen, OP_POP); // Pop the object
                codegen_emit_op(codegen, OP_PUSH_UNDEFINED);
                
                // 12. Patch end jumps
                codegen_patch_jump(codegen, end_jump);
                codegen_patch_jump(codegen, null_end_jump);
            } else {
                // Normal property access: obj.prop
                codegen_emit_expression(codegen, member_node->object);
                size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member_node->property));
                codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
                codegen_emit_op(codegen, OP_GET_PROPERTY);
            }
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
            
            if (assign->target->type == AST_IDENTIFIER) {
                // Variable assignment: var = value
                // Generate the value first
                codegen_emit_expression(codegen, assign->value);
                
                // For expressions, we need to duplicate the value so it remains on stack
                codegen_emit_op(codegen, OP_DUP);
                
                ast_identifier* var = (ast_identifier*)assign->target;
                
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
                    chunk_add_debug_info(codegen->chunk, assign->base.line, assign->base.column);
                    codegen_emit_op_operand(codegen, OP_SET_GLOBAL, (uint16_t)constant);
                }
                
            } else if (assign->target->type == AST_MEMBER) {
                // Object property assignment: obj.prop = value or obj?.prop = value
                ast_member* member = (ast_member*)assign->target;
                
                if (member->is_optional) {
                    // Optional assignment should probably not be allowed
                    codegen_error(codegen, "Cannot use optional chaining in assignment target");
                } else {
                    // Generate in order: object, property_name, value
                    codegen_emit_expression(codegen, member->object);
                    
                    size_t property_constant = chunk_add_constant(codegen->chunk, make_string(member->property));
                    codegen_emit_op_operand(codegen, OP_PUSH_CONSTANT, (uint16_t)property_constant);
                    
                    codegen_emit_expression(codegen, assign->value);
                    
                    // Stack: [object, property_name, value] - matches OP_SET_PROPERTY
                    // OP_SET_PROPERTY pops [object, property_name, value] and pushes the assigned value back
                    codegen_emit_op(codegen, OP_SET_PROPERTY);
                }
                
            } else if (assign->target->type == AST_CALL) {
                // Array element assignment: arr(index) = value
                ast_call* call = (ast_call*)assign->target;
                
                // Validate that this is array indexing (exactly one argument)
                if (call->arg_count != 1) {
                    codegen_error(codegen, "Array assignment requires exactly one index argument");
                    break;
                }
                
                // Generate in order: array, index, value
                codegen_emit_expression(codegen, call->function);      // [array]
                codegen_emit_expression(codegen, call->arguments[0]);  // [array, index]  
                codegen_emit_expression(codegen, assign->value);       // [array, index, value]
                
                // OP_SET_INDEX pops [array, index, value] and pushes the assigned value back
                // This is perfect for expression contexts
                codegen_emit_op(codegen, OP_SET_INDEX);
                
            } else {
                codegen_error(codegen, "Invalid assignment target");
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

        case AST_MATCH:
            codegen_emit_match(codegen, (ast_match*)expr);
            break;

        case AST_BLOCK:
            codegen_emit_block_expression(codegen, (ast_block*)expr);
            break;
            
        case AST_WHILE:
            codegen_emit_while(codegen, (ast_while*)expr);
            break;
            
        case AST_FOR:
            codegen_emit_for(codegen, (ast_for*)expr);
            break;
            
        case AST_DO_WHILE:
            codegen_emit_do_while(codegen, (ast_do_while*)expr);
            break;
            
        case AST_LOOP:
            codegen_emit_infinite_loop(codegen, (ast_loop*)expr);
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
            // If used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_MATCH:
            codegen_emit_match(codegen, (ast_match*)stmt);
            break;
            
        case AST_WHILE:
            codegen_emit_while(codegen, (ast_while*)stmt);
            // While used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_FOR:
            codegen_emit_for(codegen, (ast_for*)stmt);
            // For used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_DO_WHILE:
            codegen_emit_do_while(codegen, (ast_do_while*)stmt);
            // Do-while used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_LOOP:
            codegen_emit_infinite_loop(codegen, (ast_loop*)stmt);
            // Infinite loop used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
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
            // Assignment used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_COMPOUND_ASSIGNMENT:
            codegen_emit_compound_assignment(codegen, (ast_compound_assignment*)stmt);
            // Compound assignment used as statement should discard its result value
            codegen_emit_op(codegen, OP_POP);
            break;
            
        case AST_IMPORT:
            codegen_emit_import(codegen, (ast_import*)stmt);
            break;
            
        case AST_PACKAGE:
            codegen_emit_package(codegen, (ast_package*)stmt);
            break;
            
        case AST_DATA_DECLARATION:
            codegen_emit_data_declaration(codegen, (ast_data_declaration*)stmt);
            break;
            
        default:
            codegen_error(codegen, "Unknown statement type");
            break;
    }
}