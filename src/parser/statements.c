#include "parser_internal.h"
#include <stdlib.h>
#include <string.h>

// Parse statement
ast_node* parse_statement(parser_t* parser) {
    if (parser_match(parser, TOKEN_DO)) {
        return parse_do_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_WHILE)) {
        return parse_while_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_FOR)) {
        return parse_for_statement(parser);
    }
    
    if (parser_match(parser, TOKEN_LOOP)) {
        return parse_loop_statement(parser);
    }
    
    
    if (parser_match(parser, TOKEN_RETURN)) {
        return parse_return_statement(parser);
    }
    
    return parse_expression_statement(parser);
}

// Parse indented block of statements
ast_node* parse_indented_block(parser_t* parser) {
    // Skip optional newline before indent
    parser_match(parser, TOKEN_NEWLINE);
    
    if (!parser_match(parser, TOKEN_INDENT)) {
        parser_error_at_current(parser, "Expected indented block.");
        return NULL;
    }
    
    // Parse statements until we hit a DEDENT
    ast_node** statements = NULL;
    size_t statement_count = 0;
    size_t statement_capacity = 0;
    
    while (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF)) {
        // Skip newlines between statements
        while (parser_match(parser, TOKEN_NEWLINE));
        
        if (parser_check(parser, TOKEN_DEDENT)) break;
        
        ast_node* stmt = parse_declaration(parser);
        
        if (statement_count >= statement_capacity) {
            size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
            statements = realloc(statements, sizeof(ast_node*) * new_capacity);
            statement_capacity = new_capacity;
        }
        
        statements[statement_count++] = stmt;
        
        // Skip optional trailing newlines
        while (parser_match(parser, TOKEN_NEWLINE));
    }
    
    parser_consume(parser, TOKEN_DEDENT, "Expected dedent after block.");
    
    // Validate that the block ends with an expression
    if (statement_count == 0) {
        // Empty block - treat as null
        return (ast_node*)ast_create_null(parser->previous.line, parser->previous.column);
    } else {
        ast_node* last_stmt = statements[statement_count - 1];
        if (last_stmt->type != AST_EXPRESSION_STMT) {
            // In lenient mode, allow variable declarations with initializers as valid block endings
            if (parser->mode == PARSER_MODE_LENIENT && last_stmt->type == AST_VAR_DECLARATION) {
                ast_var_declaration* var_decl = (ast_var_declaration*)last_stmt;
                if (var_decl->initializer) {
                    // Variable declaration with initializer is valid - it produces the initialized value
                    // Continue without error
                } else {
                    parser_error(parser, "Block expressions must end with an expression, not a statement");
                    return NULL;
                }
            } else {
                parser_error(parser, "Block expressions must end with an expression, not a statement");
                return NULL;
            }
        }
        
        // If the last expression is a block, validate it recursively
        if (last_stmt->type == AST_EXPRESSION_STMT) {
            ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
            if (!validate_block_expression(expr_stmt->expression, parser->mode)) {
                parser_error(parser, "Nested block expressions must ultimately end with a non-block expression");
                return NULL;
            }
        }
        // Variable declarations don't need recursive validation
    }
    
    return (ast_node*)ast_create_block(statements, statement_count,
                                      parser->previous.line, parser->previous.column);
}

// Parse if expression with new syntax
ast_node* parse_if_expression(parser_t* parser) {
    // Parse condition (no parentheses required)
    ast_node* condition = parse_expression(parser);
    
    ast_node* then_expr = NULL;
    ast_node* else_expr = NULL;
    
    // Check for 'then' keyword
    if (parser_match(parser, TOKEN_THEN)) {
        // Could be: 'if condition then expression' or 'if condition then\n<indent>'
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line form with 'then'
            then_expr = parse_indented_block(parser);
        } else {
            // Single-line form with 'then' - can be expression or statement
            then_expr = parse_declaration(parser);
        }
    } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form without 'then'
        then_expr = parse_indented_block(parser);
    } else {
        parser_error_at_current(parser, "Expected 'then' or indented block after if condition.");
        return NULL;
    }
    
    // Skip optional newlines before checking for 'elif' or 'else'
    while (parser_match(parser, TOKEN_NEWLINE));
    
    // Handle elif clauses by chaining them as nested if-else
    while (parser_match(parser, TOKEN_ELIF)) {
        // Parse elif condition
        ast_node* elif_condition = parse_expression(parser);
        ast_node* elif_then = NULL;
        
        // Parse elif 'then' clause
        if (parser_match(parser, TOKEN_THEN)) {
            if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
                // Multi-line elif then
                elif_then = parse_indented_block(parser);
            } else {
                // Single-line elif then - can be expression or statement
                elif_then = parse_declaration(parser);
            }
        } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line elif without 'then'
            elif_then = parse_indented_block(parser);
        } else {
            parser_error_at_current(parser, "Expected 'then' or indented block after elif condition.");
            return NULL;
        }
        
        // Skip newlines before checking for more elif/else
        while (parser_match(parser, TOKEN_NEWLINE));
        
        // Create a nested if for the elif, and set it as the else clause
        // This will be the else clause of the current if, or the else of a previous elif
        ast_node* nested_if = (ast_node*)ast_create_if(elif_condition, elif_then, NULL,
                                                       parser->previous.line, parser->previous.column);
        
        if (else_expr == NULL) {
            else_expr = nested_if;
        } else {
            // Find the deepest else clause and attach the new elif there
            ast_if* current = (ast_if*)else_expr;
            while (current->else_stmt && current->else_stmt->type == AST_IF) {
                current = (ast_if*)current->else_stmt;
            }
            current->else_stmt = nested_if;
        }
    }
    
    // Check for final 'else' clause
    if (parser_match(parser, TOKEN_ELSE)) {
        ast_node* final_else = NULL;
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line else
            final_else = parse_indented_block(parser);
        } else {
            // Single-line else - can be expression or statement
            final_else = parse_declaration(parser);
        }
        
        if (else_expr == NULL) {
            // No elif clauses, direct else
            else_expr = final_else;
        } else {
            // Find the deepest elif and attach the final else
            ast_if* current = (ast_if*)else_expr;
            while (current->else_stmt && current->else_stmt->type == AST_IF) {
                current = (ast_if*)current->else_stmt;
            }
            current->else_stmt = final_else;
        }
    }
    
    // Check for optional 'end if'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_IF, "Expected 'if' after 'end'.");
    }
    
    return (ast_node*)ast_create_if(condition, then_expr, else_expr,
                                   parser->previous.line, parser->previous.column);
}

// Validate that a block expression ultimately ends with a non-block expression
int validate_block_expression(ast_node* expr, parser_mode_t mode) {
    if (expr->type != AST_BLOCK) {
        // Non-block expression is valid
        return 1;
    }
    
    // It's a block, check its contents
    ast_block* block = (ast_block*)expr;
    
    if (block->statement_count == 0) {
        // Empty block is valid
        return 1;
    }
    
    // Last statement must be an expression statement
    ast_node* last_stmt = block->statements[block->statement_count - 1];
    if (last_stmt->type != AST_EXPRESSION_STMT) {
        // In lenient mode, allow variable declarations with initializers
        if (mode == PARSER_MODE_LENIENT && last_stmt->type == AST_VAR_DECLARATION) {
            ast_var_declaration* var_decl = (ast_var_declaration*)last_stmt;
            if (var_decl->initializer) {
                return 1; // Valid - produces the initialized value
            }
        }
        return 0; // Block ends with statement, invalid
    }
    
    // Recursively validate the last expression
    ast_expression_stmt* expr_stmt = (ast_expression_stmt*)last_stmt;
    return validate_block_expression(expr_stmt->expression, mode);
}

// Parse while statement with new syntax
ast_node* parse_while_statement(parser_t* parser) {
    // Parse condition (no parentheses required)
    ast_node* condition = parse_expression(parser);
    
    ast_node* body = NULL;
    
    // Check for 'do' keyword
    if (parser_match(parser, TOKEN_DO)) {
        // Could be: 'while condition do expression' or 'while condition do\n<indent>'
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line form with 'do'
            body = parse_indented_block(parser);
        } else {
            // Single-line form with 'do'
            body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                         parser->current.line, parser->current.column);
        }
    } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form without 'do'
        body = parse_indented_block(parser);
    } else {
        parser_error_at_current(parser, "Expected 'do' or indented block after while condition.");
        return NULL;
    }
    
    // Check for optional 'end while'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_WHILE, "Expected 'while' after 'end'.");
    }
    
    return (ast_node*)ast_create_while(condition, body,
                                      parser->previous.line, parser->previous.column);
}

// Parse for statement: for [initializer]; [condition]; [increment] [do] body [end for]
ast_node* parse_for_statement(parser_t* parser) {
    int for_line = parser->previous.line;
    int for_column = parser->previous.column;
    
    // Parse initializer (optional): var i = 0 or i = 0 or empty
    ast_node* initializer = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        if (parser_match(parser, TOKEN_VAR)) {
            // For variable declarations in for loops, we need to handle semicolon manually
            parser_consume(parser, TOKEN_IDENTIFIER, "Expected variable name.");
            char* name = token_to_string(&parser->previous);
            ast_node* init_expr = NULL;
            
            if (parser_match(parser, TOKEN_ASSIGN)) {
                init_expr = parse_expression(parser);
            }
            
            initializer = (ast_node*)ast_create_var_declaration(name, init_expr,
                                                               parser->previous.line, parser->previous.column);
        } else {
            // Could be assignment or empty
            initializer = parse_expression(parser);
        }
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after for loop initializer");
    
    // Parse condition (optional): i < 10 or empty (defaults to true)
    ast_node* condition = NULL;
    if (!parser_check(parser, TOKEN_SEMICOLON)) {
        condition = parse_expression(parser);
    }
    parser_consume(parser, TOKEN_SEMICOLON, "Expected ';' after for loop condition");
    
    // Parse increment (optional): i++ or i += 1 or empty
    ast_node* increment = NULL;
    if (!parser_check(parser, TOKEN_DO) && !parser_check(parser, TOKEN_NEWLINE) && 
        !parser_check(parser, TOKEN_INDENT) && !parser_check(parser, TOKEN_EOF)) {
        increment = parse_expression(parser);
    }
    
    // Parse body - supports multiple syntax forms
    ast_node* body = NULL;
    
    // Check for 'do' keyword
    if (parser_match(parser, TOKEN_DO)) {
        // Could be: 'for ... do expression' or 'for ... do\n<indent>'
        if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
            // Multi-line form with 'do'
            body = parse_indented_block(parser);
        } else {
            // Single-line form with 'do'
            body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                         parser->current.line, parser->current.column);
        }
    } else if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form without 'do'
        body = parse_indented_block(parser);
    } else {
        // Single-line form without 'do' (shouldn't happen with C-style syntax but handle it)
        body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                     parser->current.line, parser->current.column);
    }
    
    // Check for optional "end for" marker
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_FOR, "Expected 'for' after 'end'");
    }
    
    return (ast_node*)ast_create_for(initializer, condition, increment, body, for_line, for_column);
}

// Parse do-while statement
ast_node* parse_do_while_statement(parser_t* parser) {
    int do_line = parser->previous.line;
    int do_column = parser->previous.column;
    
    ast_node* body = NULL;
    
    // Parse body
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form: do\n<indent>statements
        // Skip optional newline before indent
        parser_match(parser, TOKEN_NEWLINE);
        
        if (!parser_match(parser, TOKEN_INDENT)) {
            parser_error_at_current(parser, "Expected indented block.");
            return NULL;
        }
        
        // Parse statements until we hit a DEDENT
        ast_node** statements = NULL;
        size_t statement_count = 0;
        size_t statement_capacity = 0;
        
        while (!parser_check(parser, TOKEN_DEDENT) && !parser_check(parser, TOKEN_EOF)) {
            // Skip newlines between statements
            while (parser_match(parser, TOKEN_NEWLINE));
            
            if (parser_check(parser, TOKEN_DEDENT)) break;
            
            ast_node* stmt = parse_declaration(parser);
            
            if (statement_count >= statement_capacity) {
                size_t new_capacity = statement_capacity == 0 ? 8 : statement_capacity * 2;
                statements = realloc(statements, sizeof(ast_node*) * new_capacity);
                statement_capacity = new_capacity;
            }
            
            statements[statement_count++] = stmt;
            
            // Skip optional trailing newlines
            while (parser_match(parser, TOKEN_NEWLINE));
        }
        
        parser_consume(parser, TOKEN_DEDENT, "Expected dedent after block.");
        
        if (statement_count == 0) {
            // Empty block
            body = (ast_node*)ast_create_null(parser->previous.line, parser->previous.column);
        } else if (statement_count == 1) {
            // Single statement
            body = statements[0];
            free(statements);
        } else {
            // Multiple statements - create block
            body = (ast_node*)ast_create_block(statements, statement_count, parser->previous.line, parser->previous.column);
        }
    } else {
        // Single-line form: do expression while condition
        body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                     parser->current.line, parser->current.column);
    }
    
    // Consume 'while' keyword
    parser_consume(parser, TOKEN_WHILE, "Expected 'while' after do body");
    
    // Parse condition (no parentheses required)
    ast_node* condition = parse_expression(parser);
    
    return (ast_node*)ast_create_do_while(body, condition, do_line, do_column);
}

// Parse infinite loop statement
ast_node* parse_loop_statement(parser_t* parser) {
    ast_node* body = NULL;
    
    // Parse the loop body - supports both single-line and multi-line forms
    if (parser_check(parser, TOKEN_NEWLINE) || parser_check(parser, TOKEN_INDENT)) {
        // Multi-line form with indented block
        body = parse_indented_block(parser);
    } else {
        // Single-line form: loop <expression>
        body = (ast_node*)ast_create_expression_stmt(parse_expression(parser),
                                                     parser->current.line, parser->current.column);
    }
    
    // Check for optional 'end loop'
    if (parser_match(parser, TOKEN_END)) {
        parser_consume(parser, TOKEN_LOOP, "Expected 'loop' after 'end'.");
    }
    
    return (ast_node*)ast_create_loop(body, parser->previous.line, parser->previous.column);
}

// Parse break statement
ast_node* parse_break_statement(parser_t* parser) {
    // Break statements are simple - just the keyword
    return (ast_node*)ast_create_break(parser->previous.line, parser->previous.column);
}

// Parse continue statement
ast_node* parse_continue_statement(parser_t* parser) {
    // Continue statements are simple - just the keyword
    return (ast_node*)ast_create_continue(parser->previous.line, parser->previous.column);
}

// Parse return statement
ast_node* parse_return_statement(parser_t* parser) {
    ast_node* value = NULL;
    
    if (!parser_check(parser, TOKEN_SEMICOLON) && !parser_check(parser, TOKEN_NEWLINE)) {
        value = parse_expression(parser);
    }
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_return(value, parser->previous.line, parser->previous.column);
}

// Parse expression statement
ast_node* parse_expression_statement(parser_t* parser) {
    ast_node* expr = parse_expression(parser);
    
    if (!parser_match(parser, TOKEN_SEMICOLON)) {
        parser_match(parser, TOKEN_NEWLINE);
    }
    
    return (ast_node*)ast_create_expression_stmt(expr, parser->previous.line, parser->previous.column);
}