#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "codegen.h"
#include "lexer.h"
#include "line_editor.h"
#include "parser.h"
#include "vm.h"
#include "cargs.h"
#include "module.h"

// Global debug flag
static int debug_mode = 0;

// Command line options
static struct cag_option options[] = {
    {
        .identifier = 'h',
        .access_letters = "h",
        .access_name = "help",
        .description = "Show this help message"
    },
    {
        .identifier = 'v',
        .access_letters = "v",
        .access_name = "version",
        .description = "Show version information"
    },
    {
        .identifier = 'd',
        .access_letters = "d",
        .access_name = "debug",
        .description = "Enable debug mode"
    },
    {
        .identifier = 't',
        .access_letters = "t",
        .access_name = "test",
        .description = "Run built-in tests"
    },
    {
        .identifier = 's',
        .access_letters = "s",
        .access_name = "script",
        .value_name = "CODE",
        .description = "Execute script code directly"
    },
    {
        .identifier = 'f',
        .access_letters = "f",
        .access_name = "file",
        .value_name = "PATH",
        .description = "Execute script from file"
    },
    {
        .identifier = 'i',
        .access_letters = "i",
        .access_name = "stdin",
        .description = "Read and execute from standard input"
    },
    {
        .identifier = 'r',
        .access_letters = "r",
        .access_name = "repl",
        .description = "Start interactive REPL (default if no other options)"
    },
    {
        .identifier = 'D',
        .access_letters = "D",
        .access_name = "disassemble",
        .value_name = "CODE",
        .description = "Disassemble script bytecode without executing"
    },
    {
        .identifier = 'I',
        .access_letters = "I",
        .access_name = "include",
        .value_name = "PATH",
        .description = "Add directory to module search path (can be used multiple times)"
    }
};

// Show help information
static void show_help(const char* program_name) {
    printf("Slate Programming Language\n");
    printf("Usage: %s [OPTIONS] [script_args...]\n", program_name);
    printf("       %s script.sl [script_args...]\n\n", program_name);
    
    cag_option_print(options, CAG_ARRAY_SIZE(options), stdout);
    
    printf("\nExamples:\n");
    printf("  %s                          # Start interactive REPL\n", program_name);
    printf("  %s script.sl arg1 arg2    # Run file with arguments (shebang-compatible)\n", program_name);
    printf("  %s -s \"print('Hello')\"       # Execute script directly\n", program_name);
    printf("  %s -f script.sl arg1 arg2 # Run file with arguments (explicit flag)\n", program_name);
    printf("  %s --stdin < input.txt       # Execute from stdin\n", program_name);
    printf("  %s --test                    # Run built-in tests\n", program_name);
    printf("  %s -D \"f(g(3))\"              # Disassemble bytecode\n", program_name);
    printf("  %s -I /path/to/modules script.sl  # Add module search path\n", program_name);
    printf("  SLATEPATH=/path/to/modules %s script.sl  # Use environment variable\n", program_name);
    printf("\nShebang usage:\n");
    printf("  #!/usr/bin/env %s\n", program_name);
    printf("  # Your Slate script here\n");
}

// Show version information
static void show_version(void) {
    printf("Slate Programming Language v1.0.0\n");
    printf("A toy programming language implementation\n");
}

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

// Disassemble function - compiles and shows bytecode without executing
static void disassemble(const char* source) {
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
    
    // Generate code
    vm_t* temp_vm = vm_create();
    codegen_t* codegen = codegen_create_with_debug(temp_vm, source);
    function_t* function = codegen_compile(codegen, program);
    
    if (codegen->had_error || !function) {
        printf("Compilation error\n");
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
        lexer_cleanup(&lexer);
        vm_destroy(temp_vm);
        return;
    }
    
    // Disassemble bytecode
    printf("=== BYTECODE ===\n");
    bytecode_chunk chunk = {
        .code = function->bytecode,
        .count = function->bytecode_length,
        .constants = function->constants,
        .constant_count = function->constant_count
    };
    chunk_disassemble(&chunk, "main");
    printf("\n");
    
    // Clean up
    function_destroy(function);
    codegen_destroy(codegen);
    ast_free((ast_node*)program);
    lexer_cleanup(&lexer);
    vm_destroy(temp_vm);
}

// Forward declaration
static void interpret_with_vm(const char* source, vm_t* vm);

// Add search paths from SLATEPATH environment variable
static void add_env_search_paths(vm_t* vm) {
    const char* slate_path = getenv("SLATEPATH");
    if (!slate_path) return;
    
    // Make a copy since we'll modify it with strtok
    char* path_copy = strdup(slate_path);
    if (!path_copy) return;
    
    // Split by colon (Unix) or semicolon (Windows)
    const char* delimiter = ":";
    #ifdef _WIN32
    delimiter = ";";
    #endif
    
    char* token = strtok(path_copy, delimiter);
    while (token != NULL) {
        // Skip empty tokens
        if (strlen(token) > 0) {
            module_add_search_path(vm, token);
        }
        token = strtok(NULL, delimiter);
    }
    
    free(path_copy);
}

// Apply both environment variable and command line search paths to a VM
static void configure_search_paths(vm_t* vm, char** include_paths, int include_count) {
    // First add current working directory (default behavior)
    module_add_search_path(vm, ".");
    
    // Then add environment paths
    add_env_search_paths(vm);
    
    // Finally add command line paths (highest priority)
    for (int i = 0; i < include_count; i++) {
        if (include_paths[i]) {
            module_add_search_path(vm, include_paths[i]);
        }
    }
}
static void interpret_with_vm_mode(const char* source, vm_t* vm, int show_undefined);
static void interpret_with_vm_mode_parser(const char* source, vm_t* vm, int show_undefined, parser_mode_t parser_mode);

static void interpret(const char* source) { interpret_with_vm(source, NULL); }

static void interpret_with_vm_mode(const char* source, vm_t* vm, int show_undefined) {
    interpret_with_vm_mode_parser(source, vm, show_undefined, PARSER_MODE_STRICT);
}

static void interpret_with_vm(const char* source, vm_t* vm) {
    interpret_with_vm_mode(source, vm, 0); // REPL mode - show undefined  
}

static void interpret_with_vm_lenient(const char* source, vm_t* vm) {
    interpret_with_vm_mode_parser(source, vm, 0, PARSER_MODE_LENIENT);
}

static void interpret_with_vm_mode_parser(const char* source, vm_t* vm, int show_undefined, parser_mode_t parser_mode) {
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
    parser_set_mode(&parser, parser_mode);

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

    vm_t* vm_to_use = vm ? vm : vm_create();
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
                print_value(vm_to_use, vm_to_use->result);
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
    
    // Skip shebang line if present
    if (bytes_read >= 2 && buffer[0] == '#' && buffer[1] == '!') {
        // Find the end of the first line
        char* newline = strchr(buffer, '\n');
        if (newline) {
            // Move content after the shebang line to the beginning
            size_t remaining = bytes_read - (newline + 1 - buffer);
            memmove(buffer, newline + 1, remaining + 1); // +1 for null terminator
        }
    }
    
    return buffer;
}

static void repl_with_args(int argc, char** argv, char** include_paths, int include_count);
static void repl(void) { repl_with_args(0, NULL, NULL, 0); }
static int should_continue_for_data_declaration(ast_program* program);

static void repl_with_args(int argc, char** argv, char** include_paths, int include_count) {
    char line[1024];
    char accumulated_input[4096] = "";
    int in_continuation = 0;

    printf("Slate v0.1.0 - A tiny programming language\n");
    printf("Type 'exit' to quit. Empty line cancels multi-line input.\n\n");

    // Create persistent VM for the REPL session with command line arguments
    vm_t* vm = vm_create_with_args(argc, argv);
    if (!vm) {
        printf("Failed to create VM\n");
        return;
    }
    
    // Set REPL context for error handling
    vm->context = CTX_INTERACTIVE;
    
    // Initialize module system
    module_system_init(vm);
    
    // Configure module search paths
    configure_search_paths(vm, include_paths, include_count);

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
                // Re-validate in strict mode before execution to catch semantic errors early
                lexer_t final_lexer;
                lexer_init(&final_lexer, accumulated_input);
                
                parser_t final_parser;
                parser_init(&final_parser, &final_lexer);
                parser_set_mode(&final_parser, PARSER_MODE_STRICT);
                
                ast_program* final_program = parse_program(&final_parser);
                
                if (final_parser.had_error || !final_program) {
                    // Show proper parser error instead of letting codegen fail
                    printf("Parse error\n");
                    lexer_cleanup(&final_lexer);
                } else {
                    // Clean up the validation parse
                    if (final_program) {
                        ast_free((ast_node*)final_program);
                    }
                    lexer_cleanup(&final_lexer);
                    
                    // Execute with lenient parsing (for consistency, though strict should work now)
                    if (debug_mode) {
                        printf("Interpreting: %s\n", accumulated_input);
                    }
                    
                    // Wrap execution in error handling
                    if (setjmp(vm->trap) == 0) {
                        interpret_with_vm(accumulated_input, vm);  // Use normal strict mode for execution
                    } else {
                        // Error was handled by runtime_error - reset VM state for REPL
                        vm->stack_top = vm->stack;  // Clear stack
                        vm->frame_count = 0;        // Reset frame count
                        
                        // Error message already printed by runtime_error
                    }
                }
                
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
        
        // Use lenient mode during continuation, strict mode otherwise
        if (in_continuation) {
            parser_set_mode(&parser, PARSER_MODE_LENIENT);
        } else {
            parser_set_mode(&parser, PARSER_MODE_STRICT);
        }

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

        // Check if this is a data declaration that should trigger continuation mode
        int should_continue = should_continue_for_data_declaration(program);

        if (program) {
            ast_free((ast_node*)program);
        }

        // If this is a data declaration, enter continuation mode
        if (should_continue && !in_continuation) {
            in_continuation = 1;
            lexer_cleanup(&lexer);
            continue;
        }

        // If not in continuation mode, this was a complete single-line input
        if (!in_continuation) {
            if (debug_mode) {
                printf("Interpreting: %s\n", accumulated_input);
            }
            
            // Wrap execution in error handling
            if (setjmp(vm->trap) == 0) {
                interpret_with_vm(accumulated_input, vm);
            } else {
                // Error was handled by runtime_error - reset VM state for REPL
                vm->stack_top = vm->stack;  // Clear stack
                vm->frame_count = 0;        // Reset frame count
                
                // Error message already printed by runtime_error
            }
            
            accumulated_input[0] = '\0';
            printf("\n");
        }
        // If in continuation mode, just continue accumulating until empty line

        lexer_cleanup(&lexer);
    }

    // Cleanup the persistent VM
    vm_destroy(vm);
}

// Helper function to check if a parsed program contains data declarations that should trigger continuation mode
static int should_continue_for_data_declaration(ast_program* program) {
    if (!program || !program->statements) return 0;
    
    for (int i = 0; i < program->statement_count; i++) {
        ast_node* stmt = program->statements[i];
        if (stmt && stmt->type == AST_DATA_DECLARATION) {
            // For data declarations, always enter continuation mode to allow cases/methods
            return 1;
        }
    }
    return 0;
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
    // Parse command line arguments using cargs
    const char* script_file = NULL;
    int use_stdin = 0;
    const char* script_content = NULL;
    const char* disassemble_content = NULL;
    int start_repl = 0;
    
    // Collect include paths
    char** include_paths = NULL;
    int include_count = 0;
    
    // Check if first argument is a file path (for shebang support)
    // This happens when slate is used as: slate script.sl [args...]
    if (argc > 1 && argv[1][0] != '-') {
        // First argument is not a flag, treat it as a script file
        script_file = argv[1];
        
        // Pass remaining arguments to the script
        char** script_argv = (argc > 2) ? &argv[2] : NULL;
        int script_argc = (argc > 2) ? argc - 2 : 0;
        
        // For shebang mode, we don't have include paths yet, so use basic setup
        // Run the script file
        char* source = read_file(script_file);
        if (source) {
            vm_t* vm = vm_create_with_args(script_argc, script_argv);
            vm->context = CTX_SCRIPT;
            module_system_init(vm);
            module_add_search_path(vm, ".");  // Current directory
            add_env_search_paths(vm); // Environment paths
            interpret_with_vm_mode(source, vm, 0); // Hide undefined results
            vm_destroy(vm);
            free(source);
        }
        return 0;
    }
    
    cag_option_context context;
    cag_option_init(&context, options, CAG_ARRAY_SIZE(options), argc, argv);
    
    while (cag_option_fetch(&context)) {
        char identifier = cag_option_get_identifier(&context);
        switch (identifier) {
            case 'h':
                show_help(argv[0]);
                return 0;
            case 'v':
                show_version();
                return 0;
            case 'd':
                debug_mode = 1;
                break;
            case 't':
                run_tests();
                return 0;
            case 's':
                script_content = cag_option_get_value(&context);
                break;
            case 'f':
                script_file = cag_option_get_value(&context);
                break;
            case 'i':
                use_stdin = 1;
                break;
            case 'r':
                start_repl = 1;
                break;
            case 'D':
                disassemble_content = cag_option_get_value(&context);
                break;
            case 'I':
                // Add include path
                include_paths = realloc(include_paths, sizeof(char*) * (include_count + 1));
                if (include_paths) {
                    include_paths[include_count] = strdup(cag_option_get_value(&context));
                    include_count++;
                }
                break;
            case '?':
                // Invalid option
                cag_option_print_error(&context, stdout);
                show_help(argv[0]);
                return 1;
        }
    }

    // Get remaining non-option arguments (script arguments)
    int remaining_index = cag_option_get_index(&context);
    char** script_argv = (remaining_index < argc) ? &argv[remaining_index] : NULL;
    int script_argc = (remaining_index < argc) ? argc - remaining_index : 0;
    
    // Validation: ensure only one execution mode is specified
    int execution_modes = 0;
    if (use_stdin) execution_modes++;
    if (script_content) execution_modes++;
    if (script_file) execution_modes++;
    if (disassemble_content) execution_modes++;
    if (start_repl) execution_modes++;
    
    if (execution_modes > 1) {
        fprintf(stderr, "Error: Only one execution mode can be specified (--stdin, --script, --file, --disassemble, or --repl)\n");
        show_help(argv[0]);
        return 1;
    }
    
    if (disassemble_content) {
        // Disassemble the provided code
        disassemble(disassemble_content);
    } else if (use_stdin) {
        // Read and interpret from stdin with result display
        char* source = read_stdin();
        if (source) {
            // Create a shared VM to maintain state and show results
            vm_t* vm = vm_create_with_args(script_argc, script_argv);
            vm->context = CTX_INTERACTIVE;  // Set REPL context to allow redeclaration
            module_system_init(vm);
            configure_search_paths(vm, include_paths, include_count);

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
        vm_t* vm = vm_create_with_args(script_argc, script_argv);
        vm->context = CTX_SCRIPT;  // Set script context for --script option too
        module_system_init(vm);
        configure_search_paths(vm, include_paths, include_count);
        interpret_with_vm_mode(script_content, vm, 1);
        vm_destroy(vm);
    } else if (script_file) {
        // Run file with script arguments
        char* source = read_file(script_file);
        if (source) {
            vm_t* vm = vm_create_with_args(script_argc, script_argv);
            vm->context = CTX_SCRIPT;  // Set script context
            module_system_init(vm);
            configure_search_paths(vm, include_paths, include_count);
            
            // In script mode, we don't use setjmp because runtime_error will call exit(1)
            // The error handler will print the error and exit directly
            interpret_with_vm_mode(source, vm, 0); // Hide undefined results
            
            vm_destroy(vm);
            free(source);
        }
    } else {
        // No execution mode specified or explicit --repl - start REPL
        repl_with_args(script_argc, script_argv, include_paths, include_count);
    }

    // Clean up include paths
    for (int i = 0; i < include_count; i++) {
        free(include_paths[i]);
    }
    free(include_paths);

    return 0;
}
