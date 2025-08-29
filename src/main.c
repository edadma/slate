#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "line_editor.h"
#include "parser.h"
#include "vm.h"

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
static void interpret_with_vm(const char* source, slate_vm* vm);
static void interpret_with_vm_mode(const char* source, slate_vm* vm, int show_undefined);

static void interpret(const char* source) { interpret_with_vm(source, NULL); }

static void interpret_with_vm(const char* source, slate_vm* vm) {
    interpret_with_vm_mode(source, vm, 0); // REPL mode - show undefined
}

static void interpret_with_vm_mode(const char* source, slate_vm* vm, int show_undefined) {
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
    codegen_t* codegen = codegen_create_with_debug(vm, source);
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

    slate_vm* vm_to_use = vm ? vm : vm_create();
    vm_result result = vm_execute(vm_to_use, function);

    if (result == VM_OK) {
        if (debug_mode) {
            printf("Execution completed successfully\n");
        }

        // Print the result register value (value of the last statement)
        if (vm) {
            // Show result based on mode
            if (show_undefined || vm_to_use->result.type != VAL_UNDEFINED) {
                printf("Result: ");
                print_value(vm_to_use->result);
                printf("\n");
            }
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

static void repl_with_args(int argc, char** argv);
static void repl(void) { repl_with_args(0, NULL); }

static void repl_with_args(int argc, char** argv) {
    char line[1024];
    char accumulated_input[4096] = "";
    int in_continuation = 0;

    printf("Slate v0.1.0 - A tiny programming language\n");
    printf("Type 'exit' to quit. Empty line cancels multi-line input.\n\n");

    // Create persistent VM for the REPL session with command line arguments
    slate_vm* vm = vm_create_with_args(argc, argv);
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
            if (fgets(error_msg, sizeof(error_msg), error_capture) && strstr(error_msg, "Error at end")) {

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
                in_continuation = 0; // Exit continuation mode on syntax error
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
    interpret("[1, 2, 3, 4].length();");
    printf("\n");

    // Test 8: String indexing (strings as functions)
    printf("--- Test 8: String indexing (strings as functions) ---\n");
    interpret("\"Hello\"(0);");
    printf("\n");

    // Test 9: String length
    printf("--- Test 9: String length ---\n");
    interpret("\"World\".length();");
    printf("\n");

    // Test 10: Array method tests
    printf("--- Test 10: Array Methods ---\n");
    
    // Array isEmpty and nonEmpty
    printf("Array isEmpty/nonEmpty tests:\n");
    interpret("[].isEmpty();");  // Should be true
    interpret("[].nonEmpty();"); // Should be false
    interpret("[1, 2].isEmpty();"); // Should be false
    interpret("[1, 2].nonEmpty();"); // Should be true
    printf("\n");
    
    // Array push and pop
    printf("Array push/pop tests:\n");
    interpret("var arr = [1, 2]; arr.push(3); arr.length(); arr.pop(); arr.length();"); // Test push/pop sequence
    interpret("[].pop();"); // Should return null
    printf("\n");
    
    // Array indexOf and contains
    printf("Array indexOf/contains tests:\n");
    interpret("[1, 2, 3].indexOf(2);"); // Should return 1
    interpret("[1, 2, 3].indexOf(5);"); // Should return -1
    interpret("[1, 2, 3].contains(2);"); // Should return true
    interpret("[1, 2, 3].contains(5);"); // Should return false
    printf("\n");

    // Test 11: String method tests
    printf("--- Test 11: String Methods ---\n");
    
    // String isEmpty and nonEmpty
    printf("String isEmpty/nonEmpty tests:\n");
    interpret("\"\".isEmpty();");    // Should be true
    interpret("\"\".nonEmpty();");   // Should be false
    interpret("\"hello\".isEmpty();");  // Should be false
    interpret("\"hello\".nonEmpty();"); // Should be true
    printf("\n");
}

// Read from stdin until EOF
static char* read_stdin(void) {
    size_t capacity = 1024;
    size_t length = 0;
    char* buffer = malloc(capacity);
    if (!buffer)
        return NULL;

    int c;
    while ((c = fgetc(stdin)) != EOF) {
        if (length + 1 >= capacity) {
            capacity *= 2;
            char* new_buffer = realloc(buffer, capacity);
            if (!new_buffer) {
                free(buffer);
                return NULL;
            }
            buffer = new_buffer;
        }
        buffer[length++] = c;
    }

    buffer[length] = '\0';
    return buffer;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    char* script_file = NULL;
    int use_stdin = 0;
    char* script_content = NULL;
    int script_args_start = -1; // Index where script arguments begin

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--debug") == 0) {
            debug_mode = 1;
        } else if (strcmp(argv[i], "--test") == 0) {
            run_tests();
            return 0;
        } else if (strcmp(argv[i], "--stdin") == 0 || strcmp(argv[i], "--eval") == 0) {
            use_stdin = 1;
        } else if (strcmp(argv[i], "--script") == 0) {
            // Next argument should be the script content
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: --script requires a script argument\n");
                return 1;
            }
            script_content = argv[i + 1];
            i++; // Skip the next argument since we consumed it
            // All remaining arguments are script arguments
            if (i + 1 < argc) {
                script_args_start = i + 1;
            }
            break; // Stop parsing, rest are script args
        } else if (strcmp(argv[i], "-f") == 0) {
            // Next argument should be the filename
            if (i + 1 >= argc) {
                fprintf(stderr, "Error: -f requires a filename argument\n");
                return 1;
            }
            script_file = argv[i + 1];
            i++; // Skip the next argument since we consumed it
            // All remaining arguments are script arguments
            if (i + 1 < argc) {
                script_args_start = i + 1;
            }
            break; // Stop parsing, rest are script args
        } else if (argv[i][0] != '-') {
            // Non-switch arguments after switches - these are script arguments
            script_args_start = i;
            break;
        } else {
            fprintf(stderr, "Unknown option: %s\n", argv[i]);
            fprintf(stderr,
                    "Usage: %s [--debug] [--stdin] [--script <code>] [-f <file>] [script_args...] or %s --test\n",
                    argv[0], argv[0]);
            return 1;
        }
    }

    // Create script arguments for VM (either all args or subset starting from script_args_start)
    char** script_argv = NULL;
    int script_argc = 0;

    if (script_args_start != -1) {
        // Script arguments start at script_args_start
        script_argc = argc - script_args_start;
        script_argv = &argv[script_args_start];
    } else {
        // No script arguments
        script_argc = 0;
        script_argv = NULL;
    }
    if (use_stdin) {
        // Read and interpret from stdin with result display
        char* source = read_stdin();
        if (source) {
            // Create a shared VM to maintain state and show results
            slate_vm* vm = vm_create_with_args(script_argc, script_argv);

            // Split by lines and interpret each one
            char* line = strtok(source, "\n");
            while (line) {
                if (strlen(line) > 0) { // Skip empty lines
                    printf("> %s\n", line);
                    interpret_with_vm_mode(line, vm, 1);
                }
                line = strtok(NULL, "\n");
            }

            vm_destroy(vm);
            free(source);
        }
    } else if (script_content) {
        // Execute script content directly with result display
        slate_vm* vm = vm_create_with_args(script_argc, script_argv);
        interpret_with_vm_mode(script_content, vm, 1);
        vm_destroy(vm);
    } else if (script_file) {
        // Run file with script arguments
        char* source = read_file(script_file);
        if (source) {
            slate_vm* vm = vm_create_with_args(script_argc, script_argv);
            interpret_with_vm_mode(source, vm, 0); // Hide undefined results
            vm_destroy(vm);
            free(source);
        }
    } else {
        // No file specified - start REPL
        repl_with_args(script_argc, script_argv);
    }

    return 0;
}
