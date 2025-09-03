#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

// Parse declaration
ast_node* parse_declaration(parser_t* parser) {
    if (parser_match(parser, TOKEN_IMPORT)) {
        return parse_import_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_PACKAGE)) {
        return parse_package_statement(parser);
    }
    
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

// Parse import statement: import module.path.{spec1, spec2 => alias} or import module.path._
ast_node* parse_import_statement(parser_t* parser) {
    // Parse the module path (e.g., "examples.modules.math")
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_error_at_current(parser, "Expected module path after 'import'");
        return NULL;
    }
    
    // Build the full module path by concatenating identifiers separated by dots
    char* module_path = NULL;
    size_t path_length = 0;
    size_t path_capacity = 0;
    int is_wildcard = 0;
    
    // Helper macro to append to module path
    #define append_to_path(segment) do { \
        size_t segment_len = strlen(segment); \
        size_t needed = path_length + segment_len + 1; \
        if (needed > path_capacity) { \
            path_capacity = needed * 2; \
            module_path = realloc(module_path, path_capacity); \
        } \
        if (path_length > 0) { \
            module_path[path_length++] = '.'; \
        } \
        strcpy(module_path + path_length, segment); \
        path_length += segment_len; \
    } while(0)
    
    // Parse first identifier
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected module path");
    char* segment = token_to_string(&parser->previous);
    append_to_path(segment);
    free(segment);
    
    // Parse additional path segments
    while (parser_match(parser, TOKEN_DOT)) {
        if (parser_check(parser, TOKEN_LEFT_BRACE)) {
            // We hit a { which means we're at the specifier part
            break;
        }
        
        if (!parser_check(parser, TOKEN_IDENTIFIER) && !parser_check(parser, TOKEN_MULTIPLY)) {
            parser_error_at_current(parser, "Expected identifier, '{', or '_' after '.'");
            free(module_path);
            return NULL;
        }
        
        if (parser_match(parser, TOKEN_IDENTIFIER)) {
            char* next_segment = token_to_string(&parser->previous);
            // Check if this is a wildcard specifier
            if (strcmp(next_segment, "_") == 0) {
                // This is a wildcard import
                is_wildcard = 1;
                free(next_segment);
                break;
            }
            append_to_path(next_segment);
            free(next_segment);
        } else {
            break; // We hit something else, exit the path-building loop
        }
    }
    
    // Parse import specifiers
    import_specifier* specifiers = NULL;
    size_t specifier_count = 0;
    
    // Check for selective imports after we've built the module path
    if (parser_match(parser, TOKEN_LEFT_BRACE)) {
        // Parse specific imports: {name1, name2 => alias, name3}
        do {
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected identifier in import specifier");
            char* name = token_to_string(&parser->previous);
            char* alias = NULL;
            
            // Check for rename (=>)
            if (parser_match(parser, TOKEN_FAT_ARROW)) {
                parser_consume(parser, TOKEN_IDENTIFIER, "Expected alias after '=>'");
                alias = token_to_string(&parser->previous);
            }
            
            // Add to specifiers array
            specifiers = realloc(specifiers, sizeof(import_specifier) * (specifier_count + 1));
            specifiers[specifier_count].name = name;
            specifiers[specifier_count].alias = alias;
            specifier_count++;
            
        } while (parser_match(parser, TOKEN_COMMA));
        
        parser_consume(parser, TOKEN_RIGHT_BRACE, "Expected '}' after import specifiers");
    } else if (!is_wildcard) {
        // No specifier and not a wildcard means import the whole module as a namespace
        // This creates a namespace object, not a wildcard import
        // (Don't reset is_wildcard if it was already detected as a wildcard)
    }
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_import(module_path, specifiers, specifier_count, is_wildcard,
                                       parser->previous.line, parser->previous.column);
    #undef append_to_path
}

// Parse package statement: package module.path.name
ast_node* parse_package_statement(parser_t* parser) {
    // Parse the package path (similar to import but simpler - no specifiers)
    if (!parser_check(parser, TOKEN_IDENTIFIER)) {
        parser_error_at_current(parser, "Expected package name after 'package'");
        return NULL;
    }
    
    // Build the full package path by concatenating identifiers separated by dots
    char* package_path = NULL;
    size_t path_length = 0;
    size_t path_capacity = 0;
    
    // Helper macro to append to package path
    #define append_to_path(segment) do { \
        size_t segment_len = strlen(segment); \
        size_t needed = path_length + segment_len + 1; \
        if (needed > path_capacity) { \
            path_capacity = needed * 2; \
            package_path = realloc(package_path, path_capacity); \
        } \
        if (path_length > 0) { \
            package_path[path_length++] = '.'; \
        } \
        strcpy(package_path + path_length, segment); \
        path_length += segment_len; \
    } while(0)
    
    // Parse identifiers separated by dots
    do {
        parser_consume(parser, TOKEN_IDENTIFIER, "Expected package path identifier");
        char* segment = token_to_string(&parser->previous);
        append_to_path(segment);
        free(segment);
    } while (parser_match(parser, TOKEN_DOT));
    
    // Allow semicolon or newline to terminate statement
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_package(package_path,
                                        parser->previous.line, parser->previous.column);
}