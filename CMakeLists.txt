cmake_minimum_required(VERSION 4.0)
project(bit C)

set(CMAKE_C_STANDARD 11)

add_executable(bit 
    main.c 
    lexer.c 
    parser.c 
    ast.c 
    codegen.c 
    vm.c
)
