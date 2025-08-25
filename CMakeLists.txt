cmake_minimum_required(VERSION 3.10)
project(bitty C)

set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(include)

# Main executable
add_executable(bitty 
    src/main.c 
    src/lexer.c 
    src/parser.c 
    src/ast.c 
    src/codegen.c 
    src/vm.c
    src/builtins.c
    src/library_impl.c
    src/line_editor.c
)

# Link math library for main executable
target_link_libraries(bitty m)

# Tests executable (using Unity framework)
option(BUILD_TESTS "Build unit tests" ON)

if(BUILD_TESTS)
    # Download Unity if not present
    if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/unity")
        file(DOWNLOAD 
            "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.c"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/unity/unity.c")
        file(DOWNLOAD 
            "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/unity/unity.h")
        file(DOWNLOAD 
            "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity_internals.h"
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/unity/unity_internals.h")
    endif()
    
    include_directories(tests/unity)
    
    add_executable(bitty_tests
        tests/unity/unity.c
        tests/test_main.c
        tests/test_lexer.c
        tests/test_parser.c
        tests/test_vm.c
        tests/test_conditionals.c
        tests/test_while_loops.c
        tests/test_builtins.c
        tests/test_integers.c
        tests/test_arithmetic.c
        src/lexer.c
        src/parser.c
        src/ast.c
        src/codegen.c
        src/vm.c
        src/builtins.c
        src/library_impl.c
    )
    
    # Link math library for tests executable
    target_link_libraries(bitty_tests m)
    
    # Define Unity config for all test files
    target_compile_definitions(bitty_tests PRIVATE UNITY_INCLUDE_CONFIG_H)
    
    enable_testing()
    add_test(NAME bitty_tests COMMAND bitty_tests)
endif()
