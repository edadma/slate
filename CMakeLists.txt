cmake_minimum_required(VERSION 4.0)
project(bit C)

set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(include)

# Main executable
add_executable(bit 
    src/main.c 
    src/lexer.c 
    src/parser.c 
    src/ast.c 
    src/codegen.c 
    src/vm.c
)

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
    
    add_executable(bit_tests
        tests/unity/unity.c
        tests/test_main.c
        tests/test_lexer.c
        tests/test_parser.c
        tests/test_vm.c
        src/lexer.c
        src/parser.c
        src/ast.c
        src/codegen.c
        src/vm.c
    )
    
    enable_testing()
    add_test(NAME bit_tests COMMAND bit_tests)
endif()
