#include "ast.h"
#include "config.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Helper function to duplicate a string
static char* strdup_safe(const char* str) {
    if (!str) return NULL;
    size_t len = strlen(str);
    char* copy = malloc(len + 1);
    if (!copy) return NULL;
    strcpy(copy, str);
    return copy;
}

// AST creation functions
ast_integer* ast_create_integer(int32_t value, int line, int column) {
    ast_integer* node = malloc(sizeof(ast_integer));
    if (!node) return NULL;
    
    node->base.type = AST_INTEGER;
    node->base.line = line;
    node->base.column = column;
    node->value = value;
    
    return node;
}

ast_bigint* ast_create_bigint(di_int value, int line, int column) {
    ast_bigint* node = malloc(sizeof(ast_bigint));
    if (!node) return NULL;
    
    node->base.type = AST_BIGINT;
    node->base.line = line;
    node->base.column = column;
    node->value = value;
    
    return node;
}

ast_number* ast_create_float32(float value, int line, int column) {
    ast_number* node = malloc(sizeof(ast_number));
    if (!node) return NULL;
    
    node->base.type = AST_NUMBER;
    node->base.line = line;
    node->base.column = column;
    node->value.float32 = value;
    node->is_float32 = 1;
    
    return node;
}

ast_number* ast_create_float64(double value, int line, int column) {
    ast_number* node = malloc(sizeof(ast_number));
    if (!node) return NULL;
    
    node->base.type = AST_NUMBER;
    node->base.line = line;
    node->base.column = column;
    node->value.float64 = value;
    node->is_float32 = 0;
    
    return node;
}

ast_number* ast_create_number(double value, int line, int column) {
    ast_number* node = malloc(sizeof(ast_number));
    if (!node) return NULL;
    
    node->base.type = AST_NUMBER;
    node->base.line = line;
    node->base.column = column;
    
#ifdef DEFAULT_FLOAT32
    // Default to float32 in MCU mode
    node->value.float32 = (float)value;
    node->is_float32 = 1;
#else
    // Default to float64 on PC
    node->value.float64 = value;
    node->is_float32 = 0;
#endif
    
    return node;
}

ast_string* ast_create_string(const char* value, int line, int column) {
    ast_string* node = malloc(sizeof(ast_string));
    if (!node) return NULL;
    
    node->base.type = AST_STRING;
    node->base.line = line;
    node->base.column = column;
    node->value = strdup_safe(value);
    
    return node;
}

ast_template_literal* ast_create_template_literal(template_part* parts, size_t part_count, int line, int column) {
    ast_template_literal* node = malloc(sizeof(ast_template_literal));
    if (!node) return NULL;
    
    node->base.type = AST_TEMPLATE_LITERAL;
    node->base.line = line;
    node->base.column = column;
    node->parts = parts;  // Caller owns the parts array
    node->part_count = part_count;
    
    return node;
}

ast_boolean* ast_create_boolean(int value, int line, int column) {
    ast_boolean* node = malloc(sizeof(ast_boolean));
    if (!node) return NULL;
    
    node->base.type = AST_BOOLEAN;
    node->base.line = line;
    node->base.column = column;
    node->value = value;
    
    return node;
}

ast_null* ast_create_null(int line, int column) {
    ast_null* node = malloc(sizeof(ast_null));
    if (!node) return NULL;
    
    node->base.type = AST_NULL;
    node->base.line = line;
    node->base.column = column;
    
    return node;
}

ast_undefined* ast_create_undefined(int line, int column) {
    ast_undefined* node = malloc(sizeof(ast_undefined));
    if (!node) return NULL;
    
    node->base.type = AST_UNDEFINED;
    node->base.line = line;
    node->base.column = column;
    
    return node;
}

ast_identifier* ast_create_identifier(const char* name, int line, int column) {
    ast_identifier* node = malloc(sizeof(ast_identifier));
    if (!node) return NULL;
    
    node->base.type = AST_IDENTIFIER;
    node->base.line = line;
    node->base.column = column;
    node->name = strdup_safe(name);
    
    return node;
}

ast_array* ast_create_array(ast_node** elements, size_t count, int line, int column) {
    ast_array* node = malloc(sizeof(ast_array));
    if (!node) return NULL;
    
    node->base.type = AST_ARRAY;
    node->base.line = line;
    node->base.column = column;
    node->elements = elements;
    node->count = count;
    
    return node;
}

ast_binary_op* ast_create_binary_op(binary_operator op, ast_node* left, ast_node* right, int line, int column) {
    ast_binary_op* node = malloc(sizeof(ast_binary_op));
    if (!node) return NULL;
    
    node->base.type = AST_BINARY_OP;
    node->base.line = line;
    node->base.column = column;
    node->op = op;
    node->left = left;
    node->right = right;
    
    return node;
}

ast_range* ast_create_range(ast_node* start, ast_node* end, int exclusive, int line, int column) {
    ast_range* node = malloc(sizeof(ast_range));
    if (!node) return NULL;
    
    node->base.type = AST_RANGE;
    node->base.line = line;
    node->base.column = column;
    node->start = start;
    node->end = end;
    node->exclusive = exclusive;
    
    return node;
}

ast_ternary* ast_create_ternary(ast_node* condition, ast_node* true_expr, ast_node* false_expr, int line, int column) {
    ast_ternary* node = malloc(sizeof(ast_ternary));
    if (!node) return NULL;
    
    node->base.type = AST_TERNARY;
    node->base.line = line;
    node->base.column = column;
    node->condition = condition;
    node->true_expr = true_expr;
    node->false_expr = false_expr;
    
    return node;
}

ast_unary_op* ast_create_unary_op(unary_operator op, ast_node* operand, int line, int column) {
    ast_unary_op* node = malloc(sizeof(ast_unary_op));
    if (!node) return NULL;
    
    node->base.type = AST_UNARY_OP;
    node->base.line = line;
    node->base.column = column;
    node->op = op;
    node->operand = operand;
    
    return node;
}

ast_function* ast_create_function(char** parameters, size_t param_count, ast_node* body, int is_expression, int line, int column) {
    ast_function* node = malloc(sizeof(ast_function));
    if (!node) return NULL;
    
    node->base.type = AST_FUNCTION;
    node->base.line = line;
    node->base.column = column;
    node->parameters = parameters;
    node->param_count = param_count;
    node->body = body;
    node->is_expression = is_expression;
    
    return node;
}

ast_call* ast_create_call(ast_node* function, ast_node** arguments, size_t arg_count, int line, int column) {
    ast_call* node = malloc(sizeof(ast_call));
    if (!node) return NULL;
    
    node->base.type = AST_CALL;
    node->base.line = line;
    node->base.column = column;
    node->function = function;
    node->arguments = arguments;
    node->arg_count = arg_count;
    
    return node;
}

ast_member* ast_create_member(ast_node* object, const char* property, int line, int column) {
    ast_member* node = malloc(sizeof(ast_member));
    if (!node) return NULL;
    
    node->base.type = AST_MEMBER;
    node->base.line = line;
    node->base.column = column;
    node->object = object;
    node->property = strdup_safe(property);
    
    return node;
}


ast_object_literal* ast_create_object_literal(object_property* properties, size_t property_count, int line, int column) {
    ast_object_literal* node = malloc(sizeof(ast_object_literal));
    if (!node) return NULL;
    
    node->base.type = AST_OBJECT_LITERAL;
    node->base.line = line;
    node->base.column = column;
    node->properties = properties;
    node->property_count = property_count;
    
    return node;
}

ast_var_declaration* ast_create_var_declaration(const char* name, ast_node* initializer, int is_immutable, int line, int column) {
    ast_var_declaration* node = malloc(sizeof(ast_var_declaration));
    if (!node) return NULL;
    
    node->base.type = AST_VAR_DECLARATION;
    node->base.line = line;
    node->base.column = column;
    node->name = strdup_safe(name);
    node->initializer = initializer;
    node->is_immutable = is_immutable;
    
    return node;
}

ast_assignment* ast_create_assignment(ast_node* target, ast_node* value, int line, int column) {
    ast_assignment* node = malloc(sizeof(ast_assignment));
    if (!node) return NULL;
    
    node->base.type = AST_ASSIGNMENT;
    node->base.line = line;
    node->base.column = column;
    node->target = target;
    node->value = value;
    
    return node;
}

ast_compound_assignment* ast_create_compound_assignment(ast_node* target, ast_node* value, binary_operator op, int line, int column) {
    ast_compound_assignment* node = malloc(sizeof(ast_compound_assignment));
    if (!node) return NULL;
    
    node->base.type = AST_COMPOUND_ASSIGNMENT;
    node->base.line = line;
    node->base.column = column;
    node->target = target;
    node->value = value;
    node->op = op;
    
    return node;
}

ast_if* ast_create_if(ast_node* condition, ast_node* then_stmt, ast_node* else_stmt, int line, int column) {
    ast_if* node = malloc(sizeof(ast_if));
    if (!node) return NULL;
    
    node->base.type = AST_IF;
    node->base.line = line;
    node->base.column = column;
    node->condition = condition;
    node->then_stmt = then_stmt;
    node->else_stmt = else_stmt;
    
    return node;
}

ast_while* ast_create_while(ast_node* condition, ast_node* body, int line, int column) {
    ast_while* node = malloc(sizeof(ast_while));
    if (!node) return NULL;
    
    node->base.type = AST_WHILE;
    node->base.line = line;
    node->base.column = column;
    node->condition = condition;
    node->body = body;
    
    return node;
}

ast_for* ast_create_for(ast_node* initializer, ast_node* condition, ast_node* increment, ast_node* body, int line, int column) {
    ast_for* node = malloc(sizeof(ast_for));
    if (!node) return NULL;
    
    node->base.type = AST_FOR;
    node->base.line = line;
    node->base.column = column;
    node->initializer = initializer;
    node->condition = condition;
    node->increment = increment;
    node->body = body;
    
    return node;
}

ast_do_while* ast_create_do_while(ast_node* body, ast_node* condition, int line, int column) {
    ast_do_while* node = malloc(sizeof(ast_do_while));
    if (!node) return NULL;
    
    node->base.type = AST_DO_WHILE;
    node->base.line = line;
    node->base.column = column;
    node->body = body;
    node->condition = condition;
    
    return node;
}

ast_loop* ast_create_loop(ast_node* body, int line, int column) {
    ast_loop* node = malloc(sizeof(ast_loop));
    if (!node) return NULL;
    
    node->base.type = AST_LOOP;
    node->base.line = line;
    node->base.column = column;
    node->body = body;
    
    return node;
}

ast_break* ast_create_break(int line, int column) {
    ast_break* node = malloc(sizeof(ast_break));
    if (!node) return NULL;
    
    node->base.type = AST_BREAK;
    node->base.line = line;
    node->base.column = column;
    
    return node;
}

ast_continue* ast_create_continue(int line, int column) {
    ast_continue* node = malloc(sizeof(ast_continue));
    if (!node) return NULL;
    
    node->base.type = AST_CONTINUE;
    node->base.line = line;
    node->base.column = column;
    
    return node;
}

ast_return* ast_create_return(ast_node* value, int line, int column) {
    ast_return* node = malloc(sizeof(ast_return));
    if (!node) return NULL;
    
    node->base.type = AST_RETURN;
    node->base.line = line;
    node->base.column = column;
    node->value = value;
    
    return node;
}

ast_expression_stmt* ast_create_expression_stmt(ast_node* expression, int line, int column) {
    ast_expression_stmt* node = malloc(sizeof(ast_expression_stmt));
    if (!node) return NULL;
    
    node->base.type = AST_EXPRESSION_STMT;
    node->base.line = line;
    node->base.column = column;
    node->expression = expression;
    
    return node;
}

ast_block* ast_create_block(ast_node** statements, size_t statement_count, int line, int column) {
    ast_block* node = malloc(sizeof(ast_block));
    if (!node) return NULL;
    
    node->base.type = AST_BLOCK;
    node->base.line = line;
    node->base.column = column;
    node->statements = statements;
    node->statement_count = statement_count;
    
    return node;
}

ast_program* ast_create_program(ast_node** statements, size_t statement_count, int line, int column) {
    ast_program* node = malloc(sizeof(ast_program));
    if (!node) return NULL;
    
    node->base.type = AST_PROGRAM;
    node->base.line = line;
    node->base.column = column;
    node->statements = statements;
    node->statement_count = statement_count;
    
    return node;
}

// AST cleanup function
void ast_free(ast_node* node) {
    if (!node) return;
    
    switch (node->type) {
        case AST_INTEGER:
        case AST_NUMBER:
        case AST_BOOLEAN:
        case AST_NULL:
        case AST_UNDEFINED:
            // No additional cleanup needed
            break;
            
        case AST_BIGINT: {
            ast_bigint* big_node = (ast_bigint*)node;
            di_release(&big_node->value); // Free the dynamic integer
            break;
        }
            
        case AST_STRING: {
            ast_string* str_node = (ast_string*)node;
            free(str_node->value);
            break;
        }
        
        case AST_TEMPLATE_LITERAL: {
            ast_template_literal* template_node = (ast_template_literal*)node;
            for (size_t i = 0; i < template_node->part_count; i++) {
                template_part* part = &template_node->parts[i];
                if (part->type == TEMPLATE_PART_TEXT) {
                    free(part->as.text);
                } else if (part->type == TEMPLATE_PART_EXPRESSION) {
                    ast_free(part->as.expression);
                }
            }
            free(template_node->parts);
            break;
        }
        
        case AST_IDENTIFIER: {
            ast_identifier* id_node = (ast_identifier*)node;
            free(id_node->name);
            break;
        }
        
        case AST_ARRAY: {
            ast_array* array_node = (ast_array*)node;
            for (size_t i = 0; i < array_node->count; i++) {
                ast_free(array_node->elements[i]);
            }
            free(array_node->elements);
            break;
        }
        
        case AST_BINARY_OP: {
            ast_binary_op* bin_node = (ast_binary_op*)node;
            ast_free(bin_node->left);
            ast_free(bin_node->right);
            break;
        }
        
        case AST_TERNARY: {
            ast_ternary* tern_node = (ast_ternary*)node;
            ast_free(tern_node->condition);
            ast_free(tern_node->true_expr);
            ast_free(tern_node->false_expr);
            break;
        }
        
        case AST_RANGE: {
            ast_range* range_node = (ast_range*)node;
            ast_free(range_node->start);
            ast_free(range_node->end);
            break;
        }
        
        case AST_UNARY_OP: {
            ast_unary_op* un_node = (ast_unary_op*)node;
            ast_free(un_node->operand);
            break;
        }
        
        case AST_FUNCTION: {
            ast_function* func_node = (ast_function*)node;
            for (size_t i = 0; i < func_node->param_count; i++) {
                free(func_node->parameters[i]);
            }
            free(func_node->parameters);
            ast_free(func_node->body);
            break;
        }
        
        case AST_CALL: {
            ast_call* call_node = (ast_call*)node;
            ast_free(call_node->function);
            for (size_t i = 0; i < call_node->arg_count; i++) {
                ast_free(call_node->arguments[i]);
            }
            free(call_node->arguments);
            break;
        }
        
        case AST_MEMBER: {
            ast_member* mem_node = (ast_member*)node;
            ast_free(mem_node->object);
            free(mem_node->property);
            break;
        }
        
        
        case AST_OBJECT_LITERAL: {
            ast_object_literal* obj_node = (ast_object_literal*)node;
            for (size_t i = 0; i < obj_node->property_count; i++) {
                free(obj_node->properties[i].key);
                ast_free(obj_node->properties[i].value);
            }
            free(obj_node->properties);
            break;
        }
        
        case AST_VAR_DECLARATION: {
            ast_var_declaration* var_node = (ast_var_declaration*)node;
            free(var_node->name);
            ast_free(var_node->initializer);
            break;
        }
        
        case AST_ASSIGNMENT: {
            ast_assignment* assign_node = (ast_assignment*)node;
            ast_free(assign_node->target);
            ast_free(assign_node->value);
            break;
        }
        
        case AST_COMPOUND_ASSIGNMENT: {
            ast_compound_assignment* comp_assign_node = (ast_compound_assignment*)node;
            ast_free(comp_assign_node->target);
            ast_free(comp_assign_node->value);
            break;
        }
        
        case AST_IF: {
            ast_if* if_node = (ast_if*)node;
            ast_free(if_node->condition);
            ast_free(if_node->then_stmt);
            ast_free(if_node->else_stmt);
            break;
        }
        
        case AST_WHILE: {
            ast_while* while_node = (ast_while*)node;
            ast_free(while_node->condition);
            ast_free(while_node->body);
            break;
        }
        
        case AST_FOR: {
            ast_for* for_node = (ast_for*)node;
            ast_free(for_node->initializer);
            ast_free(for_node->condition);
            ast_free(for_node->increment);
            ast_free(for_node->body);
            break;
        }
        
        case AST_DO_WHILE: {
            ast_do_while* do_while_node = (ast_do_while*)node;
            ast_free(do_while_node->body);
            ast_free(do_while_node->condition);
            break;
        }
        
        case AST_LOOP: {
            ast_loop* loop_node = (ast_loop*)node;
            ast_free(loop_node->body);
            break;
        }
        
        case AST_BREAK: {
            // No additional cleanup needed for break statements
            break;
        }
        
        case AST_CONTINUE: {
            // No additional cleanup needed for continue statements
            break;
        }
        
        case AST_RETURN: {
            ast_return* ret_node = (ast_return*)node;
            ast_free(ret_node->value);
            break;
        }
        
        case AST_EXPRESSION_STMT: {
            ast_expression_stmt* expr_node = (ast_expression_stmt*)node;
            ast_free(expr_node->expression);
            break;
        }
        
        case AST_BLOCK: {
            ast_block* block_node = (ast_block*)node;
            for (size_t i = 0; i < block_node->statement_count; i++) {
                ast_free(block_node->statements[i]);
            }
            free(block_node->statements);
            break;
        }
        
        case AST_PROGRAM: {
            ast_program* prog_node = (ast_program*)node;
            for (size_t i = 0; i < prog_node->statement_count; i++) {
                ast_free(prog_node->statements[i]);
            }
            free(prog_node->statements);
            break;
        }
    }
    
    free(node);
}

// AST node type name function
const char* ast_node_type_name(ast_node_type type) {
    switch (type) {
        case AST_INTEGER: return "INTEGER";
        case AST_NUMBER: return "NUMBER";
        case AST_BIGINT: return "BIGINT";
        case AST_STRING: return "STRING";
        case AST_BOOLEAN: return "BOOLEAN";
        case AST_NULL: return "NULL";
        case AST_UNDEFINED: return "UNDEFINED";
        case AST_IDENTIFIER: return "IDENTIFIER";
        case AST_ARRAY: return "ARRAY";
        case AST_BINARY_OP: return "BINARY_OP";
        case AST_TERNARY: return "TERNARY";
        case AST_UNARY_OP: return "UNARY_OP";
        case AST_FUNCTION: return "FUNCTION";
        case AST_CALL: return "CALL";
        case AST_MEMBER: return "MEMBER";
        case AST_OBJECT_LITERAL: return "OBJECT_LITERAL";
        case AST_VAR_DECLARATION: return "VAR_DECLARATION";
        case AST_ASSIGNMENT: return "ASSIGNMENT";
        case AST_COMPOUND_ASSIGNMENT: return "COMPOUND_ASSIGNMENT";
        case AST_IF: return "IF";
        case AST_WHILE: return "WHILE";
        case AST_FOR: return "FOR";
        case AST_DO_WHILE: return "DO_WHILE";
        case AST_LOOP: return "LOOP";
        case AST_BREAK: return "BREAK";
        case AST_CONTINUE: return "CONTINUE";
        case AST_RETURN: return "RETURN";
        case AST_EXPRESSION_STMT: return "EXPRESSION_STMT";
        case AST_BLOCK: return "BLOCK";
        case AST_PROGRAM: return "PROGRAM";
        default: return "UNKNOWN";
    }
}

// AST printing function (for debugging)
static void print_indent(int indent) {
    for (int i = 0; i < indent; i++) {
        printf("  ");
    }
}

void ast_print(ast_node* node, int indent) {
    if (!node) {
        print_indent(indent);
        printf("NULL\n");
        return;
    }
    
    print_indent(indent);
    printf("%s", ast_node_type_name(node->type));
    
    switch (node->type) {
        case AST_INTEGER: {
            ast_integer* int_node = (ast_integer*)node;
            printf(": %d\n", int_node->value);
            break;
        }
        
        case AST_NUMBER: {
            ast_number* num_node = (ast_number*)node;
            if (num_node->is_float32) {
                printf(": %.6gf\n", (double)num_node->value.float32);
            } else {
                printf(": %.6g\n", num_node->value.float64);
            }
            break;
        }
        
        case AST_BIGINT: {
            ast_bigint* big_node = (ast_bigint*)node;
            char* str = di_to_string(big_node->value, 10);
            printf(": %s\n", str ? str : "NULL");
            if (str) free(str);
            break;
        }
        
        case AST_STRING: {
            ast_string* str_node = (ast_string*)node;
            printf(": \"%s\"\n", str_node->value ? str_node->value : "");
            break;
        }
        
        case AST_BOOLEAN: {
            ast_boolean* bool_node = (ast_boolean*)node;
            printf(": %s\n", bool_node->value ? "true" : "false");
            break;
        }
        
        case AST_NULL:
            printf("\n");
            break;
            
        case AST_UNDEFINED:
            printf("\n");
            break;
            
        case AST_IDENTIFIER: {
            ast_identifier* id_node = (ast_identifier*)node;
            printf(": %s\n", id_node->name ? id_node->name : "");
            break;
        }
        
        case AST_ARRAY: {
            ast_array* array_node = (ast_array*)node;
            printf(" [%zu elements]\n", array_node->count);
            for (size_t i = 0; i < array_node->count; i++) {
                ast_print(array_node->elements[i], indent + 1);
            }
            break;
        }
        
        case AST_BINARY_OP: {
            ast_binary_op* bin_node = (ast_binary_op*)node;
            const char* op_names[] = {"+", "-", "*", "/", "mod", "==", "!=", "<", "<=", ">", ">=", "&&", "||"};
            printf(": %s\n", op_names[bin_node->op]);
            ast_print(bin_node->left, indent + 1);
            ast_print(bin_node->right, indent + 1);
            break;
        }
        
        case AST_TERNARY: {
            ast_ternary* tern_node = (ast_ternary*)node;
            printf(": ?\n");
            print_indent(indent + 1);
            printf("condition: ");
            ast_print(tern_node->condition, indent + 1);
            print_indent(indent + 1);
            printf("true: ");
            ast_print(tern_node->true_expr, indent + 1);
            print_indent(indent + 1);
            printf("false: ");
            ast_print(tern_node->false_expr, indent + 1);
            break;
        }
        
        case AST_UNARY_OP: {
            ast_unary_op* un_node = (ast_unary_op*)node;
            const char* op_names[] = {"-", "!"};
            printf(": %s\n", op_names[un_node->op]);
            ast_print(un_node->operand, indent + 1);
            break;
        }
        
        case AST_FUNCTION: {
            ast_function* func_node = (ast_function*)node;
            printf(": %zu params, %s\n", func_node->param_count, 
                   func_node->is_expression ? "expression" : "block");
            for (size_t i = 0; i < func_node->param_count; i++) {
                print_indent(indent + 1);
                printf("param: %s\n", func_node->parameters[i] ? func_node->parameters[i] : "");
            }
            printf("\n");
            print_indent(indent + 1);
            printf("body:\n");
            ast_print(func_node->body, indent + 2);
            break;
        }
        
        case AST_CALL: {
            ast_call* call_node = (ast_call*)node;
            printf(": %zu args\n", call_node->arg_count);
            print_indent(indent + 1);
            printf("function:\n");
            ast_print(call_node->function, indent + 2);
            for (size_t i = 0; i < call_node->arg_count; i++) {
                print_indent(indent + 1);
                printf("arg %zu:\n", i);
                ast_print(call_node->arguments[i], indent + 2);
            }
            break;
        }
        
        case AST_PROGRAM: {
            ast_program* prog_node = (ast_program*)node;
            printf(": %zu statements\n", prog_node->statement_count);
            for (size_t i = 0; i < prog_node->statement_count; i++) {
                ast_print(prog_node->statements[i], indent + 1);
            }
            break;
        }
        
        case AST_EXPRESSION_STMT: {
            ast_expression_stmt* expr_stmt = (ast_expression_stmt*)node;
            printf("\n");
            ast_print(expr_stmt->expression, indent + 1);
            break;
        }
        
        case AST_COMPOUND_ASSIGNMENT: {
            ast_compound_assignment* comp_assign_node = (ast_compound_assignment*)node;
            const char* op_names[] = {"+", "-", "*", "/", "mod", "**"};
            printf(": %s=\n", (comp_assign_node->op < 6) ? op_names[comp_assign_node->op] : "?");
            ast_print(comp_assign_node->target, indent + 1);
            ast_print(comp_assign_node->value, indent + 1);
            break;
        }
        
        // Add other cases as needed for debugging
        default:
            printf("\n");
            break;
    }
}