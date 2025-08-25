#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "parser.h"
#include "vm.h"
#include "line_editor.h"

// Global debug flag
static int debug_mode = 0;

static void print_tokens(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);

    printf("=== TOKENS ===\n");
    token_t token;
    do {
        token = lexer_next_token(&lexer);
        printf("%-15s '%.*s'\n", token_type_name(token.type), (int)token.length, token.start);
    } while (token.type != TOKEN_EOF);
    printf("\n");
    
    lexer_cleanup(&lexer);
}

static void print_ast(ast_node* node) {
    printf("=== AST ===\n");
    ast_print(node, 0);
    printf("\n");
}

// Forward declaration
static void interpret_with_vm(const char* source, bitty_vm* vm);

static void interpret(const char* source) { interpret_with_vm(source, NULL); }

static void interpret_with_vm(const char* source, bitty_vm* vm) {
    // Only show "Interpreting:" for file mode, not REPL (REPL handles this itself)
    if (debug_mode && !vm) {
        printf("Interpreting: %s\n", source);
    }

    // Tokenize
    lexer_t lexer;
    lexer_init(&lexer, source);

    // Parse
    parser_t parser;
    parser_init(&parser, &lexer);

    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        printf("Parse error\n");
        lexer_cleanup(&lexer);
        return;
    }

    if (debug_mode) {
        print_ast((ast_node*)program);
    }

    // Generate code with debug info for better error reporting
    codegen_t* codegen = codegen_create_with_debug(source);
    function_t* function = codegen_compile(codegen, program);

    if (codegen->had_error || !function) {
        printf("Compilation error\n");
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        return;
    }

    if (debug_mode) {
        // Disassemble bytecode
        printf("=== BYTECODE ===\n");
        bytecode_chunk chunk = {.code = function->bytecode,
                                .count = function->bytecode_length,
                                .constants = function->constants,
                                .constant_count = function->constant_count};
        chunk_disassemble(&chunk, "main");
        printf("\n");

        // Execute
        printf("=== EXECUTION ===\n");
    }
    
    bitty_vm* vm_to_use = vm ? vm : vm_create();
    vm_result result = vm_execute(vm_to_use, function);

    if (result == VM_OK) {
        if (debug_mode) {
            printf("Execution completed successfully\n");
        }

        // Print the result register value (value of the last statement)
        if (vm) {
            // In REPL mode, always show the result (including undefined)
            printf("Result: ");
            print_value(vm_to_use->result);
            printf("\n");
        }
    } else {
        printf("Execution error: %d\n", result);
    }

    // Cleanup only if we created the VM locally
    if (!vm) {
        vm_destroy(vm_to_use);
    }
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
}

static char* read_file(const char* path) {
    FILE* file = fopen(path, "rb");
    if (!file) {
        fprintf(stderr, "Could not open file \"%s\".\n", path);
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t file_size = ftell(file);
    rewind(file);

    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        fprintf(stderr, "Not enough memory to read \"%s\".\n", path);
        fclose(file);
        return NULL;
    }

    size_t bytes_read = fread(buffer, sizeof(char), file_size, file);
    if (bytes_read < file_size) {
        fprintf(stderr, "Could not read file \"%s\".\n", path);
        free(buffer);
        fclose(file);
        return NULL;
    }

    buffer[bytes_read] = '\0';
    fclose(file);
    return buffer;
}

static void repl(void) {
    char line[1024];
    char accumulated_input[4096] = "";
    int in_continuation = 0;

    printf("Bitty v0.1.0 - A tiny programming language\n");
    printf("Type 'exit' to quit. Empty line cancels multi-line input.\n\n");

    // Create persistent VM for the REPL session
    bitty_vm* vm = vm_create();
    if (!vm) {
        printf("Failed to create VM\n");
        return;
    }

    for (;;) {
        // Show appropriate prompt
        if (in_continuation) {
            printf("+ ");
        } else {
            printf("> ");
        }
        fflush(stdout);

        get_line_with_editing(line, sizeof(line));

        if (strcmp(line, "exit") == 0)
            break;

        // Handle empty line
        if (strlen(line) == 0) {
            if (in_continuation) {
                // Execute accumulated input on empty line (Python style)
                if (debug_mode) {
                    printf("Interpreting: %s\n", accumulated_input);
                }
                interpret_with_vm(accumulated_input, vm);
                accumulated_input[0] = '\0';
                in_continuation = 0;
                printf("\n");
            }
            continue;
        }

        // Accumulate input
        if (in_continuation) {
            strcat(accumulated_input, "\n");
            strcat(accumulated_input, line);
        } else {
            strcpy(accumulated_input, line);
        }

        // Try to parse the accumulated input
        lexer_t lexer;
        lexer_init(&lexer, accumulated_input);

        parser_t parser;
        parser_init(&parser, &lexer);

        // Redirect stderr to capture parser errors
        FILE* old_stderr = stderr;
        FILE* error_capture = tmpfile();
        stderr = error_capture;

        ast_program* program = parse_program(&parser);
        
        // Restore stderr
        stderr = old_stderr;

        if (parser.had_error) {
            // Check if this was an "unexpected end of input" error
            rewind(error_capture);
            char error_msg[512];
            if (fgets(error_msg, sizeof(error_msg), error_capture) && 
                strstr(error_msg, "Error at end")) {
                
                // This looks like incomplete input - enter continuation mode
                if (!in_continuation) {
                    in_continuation = 1;
                }
                fclose(error_capture);
                lexer_cleanup(&lexer);
                continue;
            } else {
                // This is a real parse error - show it and exit continuation mode
                rewind(error_capture);
                while (fgets(error_msg, sizeof(error_msg), error_capture)) {
                    printf("%s", error_msg);
                }
                fclose(error_capture);
                accumulated_input[0] = '\0';
                in_continuation = 0;  // Exit continuation mode on syntax error
                lexer_cleanup(&lexer);
                continue;
            }
        }

        // Parse succeeded - but stay in continuation mode until empty line
        fclose(error_capture);
        
        if (program) {
            ast_free((ast_node*)program);
        }

        // If not in continuation mode, this was a complete single-line input
        if (!in_continuation) {
            if (debug_mode) {
                printf("Interpreting: %s\n", accumulated_input);
            }
            interpret_with_vm(accumulated_input, vm);
            accumulated_input[0] = '\0';
            printf("\n");
        }
        // If in continuation mode, just continue accumulating until empty line
        
        lexer_cleanup(&lexer);
    }

    // Cleanup the persistent VM
    vm_destroy(vm);
}

static void run_tests(void) {
    printf("=== RUNNING TESTS ===\n\n");

    // Test 1: Simple number
    printf("--- Test 1: Number literal ---\n");
    interpret("42;");
    printf("\n");

    // Test 2: String literal
    printf("--- Test 2: String literal ---\n");
    interpret("\"Hello, World!\";");
    printf("\n");

    // Test 3: Boolean literals
    printf("--- Test 3: Boolean literals ---\n");
    interpret("true;");
    interpret("false;");
    printf("\n");

    // Test 4: Arithmetic
    printf("--- Test 4: Arithmetic expressions ---\n");
    interpret("2 + 3 * 4;");
    printf("\n");

    // Test 5: Arrays
    printf("--- Test 5: Array literals ---\n");
    interpret("[1, 2, 3];");
    printf("\n");

    // Test 6: Array indexing (arrays as functions)
    printf("--- Test 6: Array indexing (arrays as functions) ---\n");
    interpret("[10, 20, 30](1);");
    printf("\n");

    // Test 7: Array length
    printf("--- Test 7: Array length ---\n");
    interpret("[1, 2, 3, 4].length;");
    printf("\n");

    // Test 8: String indexing (strings as functions)
    printf("--- Test 8: String indexing (strings as functions) ---\n");
    interpret("\"Hello\"(0);");
    printf("\n");

    // Test 9: String length
    printf("--- Test 9: String length ---\n");
    interpret("\"World\".length;");
    printf("\n");
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    int file_arg_index = -1;
    
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strcmp(argv[i], "--test") == 0) {
            run_tests();
            return 0;
        } else if (argv[i][0] != '-') {
            // This is a filename
            if (file_arg_index == -1) {
                file_arg_index = i;
            } else {
                fprintf(stderr, "Error: Multiple input files specified\n");
                return 1;
            }
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr, "Usage: %s [--debug] [script] or %s --test\n", argv[0], argv[0]);
            return 1;
        }
    }
    
    if (file_arg_index != -1) {
        // Run file
        char* source = read_file(argv[file_arg_index]);
        if (source) {
            interpret(source);
            free(source);
        }
    } else {
        // No file specified - start REPL
        repl();
    }

    return 0;
}
