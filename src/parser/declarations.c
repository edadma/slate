#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

// Parse declaration
ast_node* parse_declaration(parser_t* parser) {
    if (parser_match(parser, TOKEN_VAR)) {
        return parse_var_declaration(parser);
    }
    
    if (parser_match(parser, TOKEN_VAL)) {
        return parse_val_declaration(parser);
    }
    
    if (parser_match(parser, TOKEN_DEF)) {
        return parse_def_declaration(parser);
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
    }
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_var_declaration(name, initializer, 0,  // 0 = var (mutable)
                                                parser->previous.line, parser->previous.column);
}

// Parse immutable variable declaration
ast_node* parse_val_declaration(parser_t* parser) {
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
    
    char* name = token_to_string(&parser->previous);
    ast_node* initializer = NULL;
    
    // val declarations must have initializers
    if (!parser_match(parser, TOKEN_ASSIGN)) {
        parser_error(parser, "Immutable variable must be initialized");
        free(name);
        return NULL;
    }
    
    initializer = parse_expression(parser);
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_var_declaration(name, initializer, 1,  // 1 = val (immutable)
                                                parser->previous.line, parser->previous.column);
}

// Parse def function declaration: def name(params) = expr or def name(params) =\n<indent>
ast_node* parse_def_declaration(parser_t* parser) {
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected function name after 'def'");
    
    char* func_name = token_to_string(&parser->previous);
    int name_line = parser->previous.line;
    int name_column = parser->previous.column;
    
    // Parse parameter list
    parser_consume(parser, TOKEN_LEFT_PAREN, "Expected '(' after function name");
    
    char** parameters = NULL;
    size_t param_count = 0;
    size_t param_capacity = 0;
    
    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
        do {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name");
            char* param = token_to_string(&parser->previous);
            
            // Grow parameters array
            if (param_count >= param_capacity) {
                param_capacity = param_capacity == 0 ? 4 : param_capacity * 2;
                parameters = realloc(parameters, param_capacity * sizeof(char*));
            }
            parameters[param_count++] = param;
            
        } while (parser_match(parser, TOKEN_COMMA));
    }
    
    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters");
    parser_consume(parser, TOKEN_ASSIGN, "Expected '=' after parameter list");
    
    // Parse function body - support both single-line and indented block forms
    ast_node* body = NULL;
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Indented block form: def name(params) =\n  <block>
        body = parse_indented_block(parser);
    } else {
        // Single-line form: def name(params) = expr
        body = parse_expression(parser);
    }
    
    // Create function AST node (both forms are treated as expressions)
    ast_node* func_node = (ast_node*)ast_create_function(parameters, param_count, body, 1,
                                                        name_line, name_column);
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    // Return this as a variable declaration with the function as initializer
    return (ast_node*)ast_create_var_declaration(func_name, func_node, 1,  // 1 = immutable (like val)
                                                name_line, name_column);
}