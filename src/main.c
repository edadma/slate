#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "lexer.h"
#include "parser.h"
#include "codegen.h"
#include "vm.h"

static void print_tokens(const char* source) {
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    printf("=== TOKENS ===\n");
    token_t token;
    do {
        token = lexer_next_token(&lexer);
        printf("%-15s '%.*s'\n", token_type_name(token.type), 
               (int)token.length, token.start);
    } while (token.type != TOKEN_EOF);
    printf("\n");
}

static void print_ast(ast_node* node) {
    printf("=== AST ===\n");
    ast_print(node, 0);
    printf("\n");
}

// Forward declaration
static void interpret_with_vm(const char* source, bitty_vm* vm);

static void interpret(const char* source) {
    interpret_with_vm(source, NULL);
}

static void interpret_with_vm(const char* source, bitty_vm* vm) {
    printf("Interpreting: %s\n", source);
    
    // Tokenize
    lexer_t lexer;
    lexer_init(&lexer, source);
    
    // Parse
    parser_t parser;
    parser_init(&parser, &lexer);
    
    ast_program* program = parse_program(&parser);
    if (parser.had_error || !program) {
        printf("Parse error\n");
        return;
    }
    
    print_ast((ast_node*)program);
    
    // Generate code with debug info for better error reporting
    codegen_t* codegen = codegen_create_with_debug(source);
    function_t* function = codegen_compile(codegen, program);
    
    if (codegen->had_error || !function) {
        printf("Compilation error\n");
        codegen_destroy(codegen);
        ast_free((ast_node*)program);
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
    
    // Execute
    printf("=== EXECUTION ===\n");
    bitty_vm* vm_to_use = vm ? vm : vm_create();
    vm_result result = vm_execute(vm_to_use, function);
    
    if (result == VM_OK) {
        printf("Execution completed successfully\n");
        
        // Print final stack state
        if (vm_to_use->stack_top > vm_to_use->stack) {
            printf("Final stack value: ");
            print_value(vm_to_use->stack[0]);
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
    
    printf("Bitty v0.1.0 - A tiny programming language\n");
    printf("Type 'exit' to quit.\n\n");
    
    // Create persistent VM for the REPL session
    bitty_vm* vm = vm_create();
    if (!vm) {
        printf("Failed to create VM\n");
        return;
    }
    
    for (;;) {
        printf("> ");
        
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        
        // Remove trailing newline
        line[strcspn(line, "\n")] = '\0';
        
        if (strcmp(line, "exit") == 0) break;
        if (strlen(line) == 0) continue;
        
        interpret_with_vm(line, vm);
        printf("\n");
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
    if (argc == 1) {
        // No arguments - run tests then REPL
        run_tests();
        repl();
    } else if (argc == 2) {
        if (strcmp(argv[1], "--test") == 0) {
            run_tests();
        } else {
            // Run file
            char* source = read_file(argv[1]);
            if (source) {
                interpret(source);
                free(source);
            }
        }
    } else {
        fprintf(stderr, "Usage: %s [script] or %s --test\n", argv[0], argv[0]);
        return 1;
    }
    
    return 0;
}
