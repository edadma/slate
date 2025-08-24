#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Helper function to extract string from token
static char* token_to_string(token_t* token) {
    if (!token || !token->start) return NULL;
    
    char* str = malloc(token->length + 1);
    if (!str) return NULL;
    
    memcpy(str, token->start, token->length);
    str[token->length] = '\0';
    
    return str;
}

// Helper function to extract number from token
static double token_to_number(token_t* token) {
    if (!token || !token->start) return 0.0;
    
    char* str = token_to_string(token);
    if (!str) return 0.0;
    
    double value = strtod(str, NULL);
    free(str);
    
    return value;
}

// Parser initialization
void parser_init(parser_t* parser, lexer_t* lexer) {
    parser->lexer = lexer;
    parser->had_error = 0;
    parser->panic_mode = 0;
    
    // Prime the parser with the first token
    parser_advance(parser);
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

// Helper function to print caret pointing to error position
static void print_error_caret(const char* source, token_t* token) {
    size_t line_length;
    const char* line_start = get_source_line(source, token->line, &line_length);
    
    if (!line_start) return;
    
    // Print the source line
    fprintf(stderr, "    %.*s\n", (int)line_length, line_start);
    
    // Calculate the column position of the token within the line
    int caret_position = (int)(token->start - line_start);
    
    // Print spaces followed by caret
    fprintf(stderr, "    ");
    for (int i = 0; i < caret_position; i++) {
        fprintf(stderr, " ");
    }
    fprintf(stderr, "^\n");
}

// Error handling
void parser_error_at(parser_t* parser, token_t* token, const char* message) {
    if (parser->panic_mode) return;
    parser->panic_mode = 1;
    
    fprintf(stderr, "[line %d] Error", token->line);
    
    if (token->type == TOKEN_EOF) {
        fprintf(stderr, " at end");
    } else if (token->type == TOKEN_ERROR) {
        // For error tokens, show the problematic character
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    } else {
        fprintf(stderr, " at '%.*s'", (int)token->length, token->start);
    }
    
    fprintf(stderr, ": %s\n", message);
    
    // Print source line with caret if we have access to source
    if (parser->lexer && parser->lexer->source) {
        print_error_caret(parser->lexer->source, token);
    }
    
    parser->had_error = 1;
}

void parser_error(parser_t* parser, const char* message) {
    parser_error_at(parser, &parser->previous, message);
}

void parser_error_at_current(parser_t* parser, const char* message) {
    parser_error_at(parser, &parser->current, message);
}

// Token consumption
void parser_advance(parser_t* parser) {
    parser->previous = parser->current;
    
    for (;;) {
        parser->current = lexer_next_token(parser->lexer);
        if (parser->current.type != TOKEN_ERROR) break;
        
        parser_error_at_current(parser, "Unexpected character");
    }
}

int parser_check(parser_t* parser, token_type_t type) {
    return parser->current.type == type;
}

int parser_match(parser_t* parser, token_type_t type) {
    if (!parser_check(parser, type)) return 0;
    parser_advance(parser);
    return 1;
}

void parser_consume(parser_t* parser, token_type_t type, const char* message) {
    if (parser->current.type == type) {
        parser_advance(parser);
        return;
    }
    
    parser_error_at_current(parser, message);
}

void parser_synchronize(parser_t* parser) {
    parser->panic_mode = 0;
    
    while (parser->current.type != TOKEN_EOF) {
        if (parser->previous.type == TOKEN_SEMICOLON) return;
        if (parser->previous.type == TOKEN_NEWLINE) return;
        
        switch (parser->current.type) {
            case TOKEN_VAR:
            case TOKEN_FUNCTION:
            case TOKEN_IF:
            case TOKEN_WHILE:
            case TOKEN_RETURN:
                return;
            default:
                // Do nothing
                break;
        }
        
        parser_advance(parser);
    }
}

// Binary operator conversion
binary_operator token_to_binary_op(token_type_t type) {
    switch (type) {
        case TOKEN_PLUS: return BIN_ADD;
        case TOKEN_MINUS: return BIN_SUBTRACT;
        case TOKEN_MULTIPLY: return BIN_MULTIPLY;
        case TOKEN_DIVIDE: return BIN_DIVIDE;
        case TOKEN_EQUAL: return BIN_EQUAL;
        case TOKEN_NOT_EQUAL: return BIN_NOT_EQUAL;
        case TOKEN_LESS: return BIN_LESS;
        case TOKEN_LESS_EQUAL: return BIN_LESS_EQUAL;
        case TOKEN_GREATER: return BIN_GREATER;
        case TOKEN_GREATER_EQUAL: return BIN_GREATER_EQUAL;
        case TOKEN_LOGICAL_AND: return BIN_LOGICAL_AND;
        case TOKEN_LOGICAL_OR: return BIN_LOGICAL_OR;
        default: return BIN_ADD; // Fallback
    }
}

// Unary operator conversion
unary_operator token_to_unary_op(token_type_t type) {
    switch (type) {
        case TOKEN_MINUS: return UN_NEGATE;
        case TOKEN_LOGICAL_NOT: return UN_NOT;
        default: return UN_NEGATE; // Fallback
    }
}

// Parse program (entry point)
ast_program* parse_program(parser_t* parser) {
    ast_node** statements = NULL;
    size_t statement_count = 0;
    size_t statement_capacity = 0;
    
    // Skip any initial newlines
    while (parser_match(parser, TOKEN_NEWLINE));
    
    while (!parser_check(parser, TOKEN_EOF)) {
        ast_node* stmt = parse_declaration(parser);
        
        if (stmt) {
            // Grow statements array if needed
            if (statement_count >= statement_capacity) {
                size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
                statements = realloc(statements, sizeof(ast_node*) * new_capacity);
                statement_capacity = new_capacity;
            }
            
            statements[statement_count++] = stmt;
        }
        
        if (parser->panic_mode) parser_synchronize(parser);
        
        // Skip newlines between statements
        while (parser_match(parser, TOKEN_NEWLINE));
    }
    
    return ast_create_program(statements, statement_count, 
                             parser->previous.line, parser->previous.column);
}

// Parse declaration
ast_node* parse_declaration(parser_t* parser) {
    if (parser_match(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser);
    }
    
    return parse_statement(parser);
}

// Parse variable declaration
ast_node* parse_var_declaration(parser_t* parser) {
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
    
    char* name = token_to_string(&parser->previous);
    ast_node* initializer = NULL;
    
    if (parser_match(parser, TOKEN_ASSIGN)) {
        initializer = parse_expression(parser);
        
        // Check if trying to assign undefined
        if (initializer && initializer->type == AST_UNDEFINED) {
            parser_error_at_current(parser, "Cannot assign 'undefined' to variable.");
            return NULL;
        }
    }
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_var_declaration(name, initializer, 
                                                parser->previous.line, parser->previous.column);
}

// Parse statement
ast_node* parse_statement(parser_t* parser) {
    if (parser_match(parser, TOKEN_IF)) {
        return parse_if_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_LEFT_BRACE)) {
        return parse_block_statement(parser);
    }
    
    return parse_expression_statement(parser);
}

// Parse if statement
ast_node* parse_if_statement(parser_t* parser) {
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'if'.");
    ast_node* condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after if condition.");
    
    ast_node* then_stmt = parse_statement(parser);
    ast_node* else_stmt = NULL;
    
    if (parser_match(parser, TOKEN_ELSE)) {
        else_stmt = parse_statement(parser);
    }
    
    return (ast_node*)ast_create_if(condition, then_stmt, else_stmt,
                                   parser->previous.line, parser->previous.column);
}

// Parse while statement
ast_node* parse_while_statement(parser_t* parser) {
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after 'while'.");
    ast_node* condition = parse_expression(parser);
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after while condition.");
    
    ast_node* body = parse_statement(parser);
    
    return (ast_node*)ast_create_while(condition, body,
                                      parser->previous.line, parser->previous.column);
}

// Parse return statement
ast_node* parse_return_statement(parser_t* parser) {
    ast_node* value = NULL;
    
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE)) {
        value = parse_expression(parser);
        
        // Check if trying to return undefined
        if (value && value->type == AST_UNDEFINED) {
            parser_error_at_current(parser, "Cannot return 'undefined' from function.");
            return NULL;
        }
    }
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_return(value, parser->previous.line, parser->previous.column);
}

// Parse block statement
ast_node* parse_block_statement(parser_t* parser) {
    ast_node** statements = NULL;
    size_t statement_count = 0;
    size_t statement_capacity = 0;
    
    while (!parser_check(parser, TOKEN_RIGHT_BRACE) && !parser_check(parser, TOKEN_EOF)) {
        // Skip newlines in blocks
        if (parser_match(parser, TOKEN_NEWLINE)) continue;
        
        ast_node* stmt = parse_declaration(parser);
        
        if (stmt) {
            if (statement_count >= statement_capacity) {
                size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
                statements = realloc(statements, sizeof(ast_node*) * new_capacity);
                statement_capacity = new_capacity;
            }
            
            statements[statement_count++] = stmt;
        }
        
        if (parser->panic_mode) parser_synchronize(parser);
    }
    
    parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after block.");
    
    return (ast_node*)ast_create_block(statements, statement_count,
                                      parser->previous.line, parser->previous.column);
}

// Parse expression statement
ast_node* parse_expression_statement(parser_t* parser) {
    ast_node* expr = parse_expression(parser);
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_expression_stmt(expr, parser->previous.line, parser->previous.column);
}

// Parse expression
ast_node* parse_expression(parser_t* parser) {
    return parse_assignment(parser);
}

// Parse assignment
ast_node* parse_assignment(parser_t* parser) {
    ast_node* expr = parse_or(parser);
    
    if (parser_match(parser, TOKEN_ASSIGN)) {
        ast_node* value = parse_assignment(parser);
        
        return (ast_node*)ast_create_assignment(expr, value,
                                               parser->previous.line, parser->previous.column);
    }
    
    return expr;
}

// Parse logical OR
ast_node* parse_or(parser_t* parser) {
    ast_node* expr = parse_and(parser);
    
    while (parser_match(parser, TOKEN_LOGICAL_OR)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_and(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse logical AND
ast_node* parse_and(parser_t* parser) {
    ast_node* expr = parse_equality(parser);
    
    while (parser_match(parser, TOKEN_LOGICAL_AND)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_equality(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse equality
ast_node* parse_equality(parser_t* parser) {
    ast_node* expr = parse_comparison(parser);
    
    while (parser_match(parser, TOKEN_EQUAL) || parser_match(parser, TOKEN_NOT_EQUAL)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_comparison(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse comparison
ast_node* parse_comparison(parser_t* parser) {
    ast_node* expr = parse_term(parser);
    
    while (parser_match(parser, TOKEN_GREATER) || parser_match(parser, TOKEN_GREATER_EQUAL) ||
           parser_match(parser, TOKEN_LESS) || parser_match(parser, TOKEN_LESS_EQUAL)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_term(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse term (+ -)
ast_node* parse_term(parser_t* parser) {
    ast_node* expr = parse_factor(parser);
    
    while (parser_match(parser, TOKEN_PLUS) || parser_match(parser, TOKEN_MINUS)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_factor(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse factor (* /)
ast_node* parse_factor(parser_t* parser) {
    ast_node* expr = parse_unary(parser);
    
    while (parser_match(parser, TOKEN_MULTIPLY) || parser_match(parser, TOKEN_DIVIDE)) {
        binary_operator op = token_to_binary_op(parser->previous.type);
        int op_line = parser->previous.line;
        int op_column = parser->previous.column;
        ast_node* right = parse_unary(parser);
        expr = (ast_node*)ast_create_binary_op(op, expr, right, op_line, op_column);
    }
    
    return expr;
}

// Parse unary (! -)
ast_node* parse_unary(parser_t* parser) {
    if (parser_match(parser, TOKEN_LOGICAL_NOT) || parser_match(parser, TOKEN_MINUS)) {
        unary_operator op = token_to_unary_op(parser->previous.type);
        ast_node* right = parse_unary(parser);
        return (ast_node*)ast_create_unary_op(op, right,
                                             parser->previous.line, parser->previous.column);
    }
    
    return parse_call(parser);
}

// Parse call
ast_node* parse_call(parser_t* parser) {
    ast_node* expr = parse_primary(parser);
    
    while (1) {
        if (parser_match(parser, TOKEN_LEFT_PAREN)) {
            expr = finish_call(parser, expr);
        } else if (parser_match(parser, TOKEN_DOT)) {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected property name after '.'.");
            char* name = token_to_string(&parser->previous);
            expr = (ast_node*)ast_create_member(expr, name, 
                                               parser->previous.line, parser->previous.column);
            free(name);
        } else {
            break;
        }
    }
    
    return expr;
}

// Finish function call
ast_node* finish_call(parser_t* parser, ast_node* callee) {
    ast_node** arguments = NULL;
    size_t arg_count = 0;
    size_t arg_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            ast_node* arg = parse_expression(parser);
            
            if (arg_count >= arg_capacity) {
                size_t new_capacity = arg_capacity == 0 ? 8 : arg_capacity * 2;
                arguments = realloc(arguments, sizeof(ast_node*) * new_capacity);
                arg_capacity = new_capacity;
            }
            
            arguments[arg_count++] = arg;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after arguments.");
    
    return (ast_node*)ast_create_call(callee, arguments, arg_count,
                                     parser->previous.line, parser->previous.column);
}

// Parse primary expressions
ast_node* parse_primary(parser_t* parser) {
    if (parser_match(parser, TOKEN_TRUE)) {
        return (ast_node*)ast_create_boolean(1, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_FALSE)) {
        return (ast_node*)ast_create_boolean(0, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_NULL)) {
        return (ast_node*)ast_create_null(parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_UNDEFINED)) {
        return (ast_node*)ast_create_undefined(parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_NUMBER)) {
        double value = token_to_number(&parser->previous);
        return (ast_node*)ast_create_number(value, parser->previous.line, parser->previous.column);
    }
    
    if (parser_match(parser, TOKEN_STRING)) {
        // Remove quotes from string
        char* str = token_to_string(&parser->previous);
        if (str && strlen(str) >= 2) {
            // Remove surrounding quotes
            memmove(str, str + 1, strlen(str) - 2);
            str[strlen(str) - 2] = '\0';
        }
        ast_node* result = (ast_node*)ast_create_string(str, parser->previous.line, parser->previous.column);
        free(str);
        return result;
    }
    
    if (parser_match(parser, TOKEN_IDENTIFIER)) {
        char* name = token_to_string(&parser->previous);
        ast_node* result = (ast_node*)ast_create_identifier(name, parser->previous.line, parser->previous.column);
        free(name);
        return result;
    }
    
    if (parser_match(parser, TOKEN_LEFT_PAREN)) {
        ast_node* expr = parse_expression(parser);
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after expression.");
        return expr;
    }
    
    if (parser_match(parser, TOKEN_LEFT_BRACKET)) {
        return parse_array(parser);
    }
    
    if (parser_match(parser, TOKEN_LEFT_BRACE)) {
        return parse_object(parser);
    }
    
    parser_error_at_current(parser, "Expected expression.");
    return NULL;
}

// Parse array literal
ast_node* parse_array(parser_t* parser) {
    ast_node** elements = NULL;
    size_t element_count = 0;
    size_t element_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_BRACKET)) {
        do {
            ast_node* element = parse_expression(parser);
            
            if (element_count >= element_capacity) {
                size_t new_capacity = element_capacity == 0 ? 8 : element_capacity * 2;
                elements = realloc(elements, sizeof(ast_node*) * new_capacity);
                element_capacity = new_capacity;
            }
            
            elements[element_count++] = element;
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_BRACKET, "Expected ']' after array elements.");
    
    return (ast_node*)ast_create_array(elements, element_count,
                                      parser->previous.line, parser->previous.column);
}

// Parse object literal  
ast_node* parse_object(parser_t* parser) {
    object_property* properties = NULL;
    size_t property_count = 0;
    size_t property_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_BRACE)) {
        do {
            // Skip newlines inside object
            while (parser_match(parser, TOKEN_NEWLINE));
            
            if (parser_check(parser, TOKEN_RIGHT_BRACE)) break;
            
            // Parse key (identifier or string)
            char* key = NULL;
            if (parser_match(parser, TOKEN_IDENTIFIER)) {
                key = token_to_string(&parser->previous);
            } else if (parser_match(parser, TOKEN_STRING)) {
                key = token_to_string(&parser->previous);
                // Remove quotes
                if (key && strlen(key) >= 2) {
                    memmove(key, key + 1, strlen(key) - 2);
                    key[strlen(key) - 2] = '\0';
                }
            } else {
                parser_error_at_current(parser, "Expected property name.");
                break;
            }
            
            parser_consume(parser, TOKEN_COLON, "Expected ':' after property name.");
            ast_node* value = parse_expression(parser);
            
            if (property_count >= property_capacity) {
                size_t new_capacity = property_capacity == 0 ? 8 : property_capacity * 2;
                properties = realloc(properties, sizeof(object_property) * new_capacity);
                property_capacity = new_capacity;
            }
            
            properties[property_count].key = key;
            properties[property_count].value = value;
            property_count++;
            
            // Skip newlines after property
            while (parser_match(parser, TOKEN_NEWLINE));
            
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after object properties.");
    
    return (ast_node*)ast_create_object_literal(properties, property_count,
                                               parser->previous.line, parser->previous.column);
}