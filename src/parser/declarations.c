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
    
    // Handle private modifier
    if (parser_match(parser, TOKEN_PRIVATE)) {
        if (parser_match(parser, TOKEN_DATA)) {
            return parse_data_declaration(parser, 1);  // is_private = 1
        } else {
            parser_error(parser, "Expected 'data' after 'private'");
            return NULL;
        }
    }
    
    if (parser_match(parser, TOKEN_DATA)) {
        return parse_data_declaration(parser, 0);  // is_private = 0
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
    
    // Check if this is a single item import (e.g., import module.item)
    // This happens when we're at end of statement (no { or other tokens) and have a dot-separated path
    if (!parser_check(parser, TOKEN_LEFT_BRACE) && !is_wildcard && 
        (parser_check(parser, TOKEN_SEMICOLON) || parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_EOF))) {
        
        // Find the last dot in the module path to split module from item
        char* last_dot = strrchr(module_path, '.');
        if (last_dot) {
            // Split the path: everything before last dot is module, after is item
            char* item_name = strdup(last_dot + 1);
            *last_dot = '\0'; // Truncate module_path at the last dot
            
            // Create a single specifier for this item
            specifiers = malloc(sizeof(import_specifier));
            specifiers[0].name = item_name;
            specifiers[0].alias = NULL; // No alias for single imports
            specifier_count = 1;
        }
    }
    
    // Check for selective imports after we've built the module path
    else if (parser_match(parser, TOKEN_LEFT_BRACE)) {
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

// Parse data declaration
ast_node* parse_data_declaration(parser_t* parser, int is_private) {
    int data_line = parser->previous.line;
    int data_column = parser->previous.column;
    
    // Parse data type name
    parser_consume(parser, TOKEN_IDENTIFIER, "Expected data type name.");
    char* name = token_to_string(&parser->previous);
    
    ast_node* shared_methods = NULL;
    ast_data_case* cases = NULL;
    size_t case_count = 0;
    char** single_constructor_params = NULL;
    size_t single_constructor_param_count = 0;
    
    // Check if this is a single-constructor data type: data Person(name, age)
    if (parser_check(parser, TOKEN_LEFT_PAREN)) {
        // Single-constructor data type
        parser_advance(parser);  // consume '('
        
        // Parse parameter list
        if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
            do {
                parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name.");
                char* param_name = token_to_string(&parser->previous);
                
                // Resize parameter array
                single_constructor_params = realloc(single_constructor_params, 
                                                   sizeof(char*) * (single_constructor_param_count + 1));
                single_constructor_params[single_constructor_param_count++] = param_name;
                
            } while (parser_match(parser, TOKEN_COMMA));
        }
        
        parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after parameters.");
    }
    
    // Parse optional shared methods and cases
    parser_match(parser, TOKEN_NEWLINE);  // Optional newline
    
    int has_indented_body = 0;
    if (parser_check(parser, TOKEN_INDENT)) {
        has_indented_body = 1;
        parser_advance(parser);  // consume INDENT
        
        // Parse method definitions and cases
        while (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF)) {
            parser_match(parser, TOKEN_NEWLINE);  // Skip newlines
            
            if (parser_check(parser, TOKEN_DEDENT)) break;
            
            if (parser_match(parser, TOKEN_CASE)) {
                // Parse case declaration
                parser_consume(parser, TOKEN_IDENTIFIER, "Expected case name.");
                char* case_name = token_to_string(&parser->previous);
                
                data_case_type case_type = DATA_CASE_SINGLETON;
                char** case_params = NULL;
                size_t case_param_count = 0;
                
                // Check for constructor parameters
                if (parser_match(parser, TOKEN_LEFT_PAREN)) {
                    case_type = DATA_CASE_CONSTRUCTOR;
                    
                    if (!parser_check(parser, TOKEN_RIGHT_PAREN)) {
                        do {
                            parser_consume(parser, TOKEN_IDENTIFIER, "Expected parameter name.");
                            char* param_name = token_to_string(&parser->previous);
                            
                            case_params = realloc(case_params, sizeof(char*) * (case_param_count + 1));
                            case_params[case_param_count++] = param_name;
                            
                        } while (parser_match(parser, TOKEN_COMMA));
                    }
                    
                    parser_consume(parser, TOKEN_RIGHT_PAREN, "Expected ')' after case parameters.");
                }
                
                // Parse case-specific methods (if any)
                ast_node* case_methods = NULL;
                parser_match(parser, TOKEN_NEWLINE);
                
                if (parser_check(parser, TOKEN_INDENT)) {
                    case_methods = parse_indented_block(parser);
                }
                
                // Add case to array
                cases = realloc(cases, sizeof(ast_data_case) * (case_count + 1));
                ast_data_case* case_node = ast_create_data_case(case_name, case_type, case_params, 
                                                               case_param_count, case_methods);
                cases[case_count] = *case_node;
                free(case_node);  // We copied the contents, free the wrapper
                case_count++;
                
            } else if (parser_match(parser, TOKEN_DEF)) {
                // This is a shared method - we need to backtrack and parse the whole block
                parser_pushback(parser);  // Put back the 'def'
                
                if (shared_methods == NULL) {
                    // Parse all remaining content as shared methods
                    shared_methods = parse_indented_block(parser);
                    break;  // The block parsing consumed everything
                }
            } else {
                parser_error(parser, "Expected 'case' or 'def' in data declaration body.");
                break;
            }
            
            parser_match(parser, TOKEN_NEWLINE);  // Skip trailing newlines
        }
        
        parser_consume(parser, TOKEN_DEDENT, "Expected dedent after data declaration body.");
    }
    
    // Optional end marker: end DataTypeName (only if there was an indented body)
    if (has_indented_body && parser_match(parser, TOKEN_END)) {
        if (parser_check(parser, TOKEN_IDENTIFIER)) {
            char* end_name = token_to_string(&parser->current);
            if (strcmp(end_name, name) != 0) {
                parser_error(parser, "End marker name doesn't match data type name");
            } else {
                parser_advance(parser);  // consume the identifier
            }
            free(end_name);  // Only free once, after the if-else
        } else {
            parser_error(parser, "Expected data type name after 'end'");
        }
    }
    
    return (ast_node*)ast_create_data_declaration(name, is_private, shared_methods, 
                                                  cases, case_count, single_constructor_params, 
                                                  single_constructor_param_count, data_line, data_column);
}