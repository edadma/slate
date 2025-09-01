cmake_minimum_required(VERSION 3.10)
project(slate C)

set(CMAKE_C_STANDARD 11)

# Include directories
include_directories(include)
include_directories(deps/cargs/include)
include_directories(deps/dynamic_string.h)
include_directories(deps/dynamic_array.h)
include_directories(deps/dynamic_object.h)
include_directories(deps/dynamic_int.h)
include_directories(deps/dynamic_buffer.h)
include_directories(deps/stb_ds.h)
include_directories(src/classes/String)
include_directories(src/classes/Array)
include_directories(src/classes/Buffer)
include_directories(src/classes/BufferBuilder)
include_directories(src/classes/BufferReader)
include_directories(src/classes/Range)
include_directories(src/classes/Iterator)
include_directories(src/classes/Value)
include_directories(src/classes/Int)

# Main executable
add_executable(slate
        src/main.c
        src/lexer.c
        src/parser.c
        src/ast.c
        src/codegen.c
        src/vm.c
        src/value.c
        src/builtins.c
        src/library_impl.c
        src/line_editor.c
        src/datetime.c
        src/classes/String/factory.c
        src/classes/String/methods.c
        src/classes/String/builder.c
        src/classes/Array/methods.c
        src/classes/Array/functional.c
        src/classes/Array/class.c
        src/classes/Buffer/methods.c
        src/classes/Buffer/factory.c
        src/classes/Buffer/class.c
        src/classes/BufferBuilder/buffer_builder.c
        src/classes/BufferReader/buffer_reader.c
        src/classes/Range/range.c
        src/classes/Iterator/iterator.c
        src/classes/Value/value.c
        src/classes/Int/int.c
        src/opcodes/op_add.c
        src/opcodes/op_subtract.c
        src/opcodes/op_multiply.c
        src/opcodes/op_divide.c
        src/opcodes/op_mod.c
        src/opcodes/op_negate.c
        src/opcodes/op_equal.c
        src/opcodes/op_not_equal.c
        src/opcodes/op_less.c
        src/opcodes/op_less_equal.c
        src/opcodes/op_greater.c
        src/opcodes/op_greater_equal.c
        src/opcodes/op_return.c
        src/opcodes/op_get_local.c
        src/opcodes/op_set_local.c
        src/opcodes/op_define_global.c
        src/opcodes/op_set_global.c
        src/opcodes/op_build_object.c
        src/opcodes/op_power.c
        src/opcodes/op_build_array.c
        src/opcodes/op_and.c
        src/opcodes/op_bitwise_and.c
        src/opcodes/op_push_constant.c
        src/opcodes/op_get_global.c
        src/opcodes/op_get_property.c
        src/opcodes/op_call.c
        src/opcodes/op_closure.c
        src/opcodes/op_set_debug_location.c
        src/opcodes/op_push_null.c
        src/opcodes/op_push_undefined.c
        src/opcodes/op_push_true.c
        src/opcodes/op_push_false.c
        src/opcodes/op_pop.c
        src/opcodes/op_dup.c
        src/opcodes/op_set_result.c
        src/opcodes/op_clear_debug_location.c
        src/opcodes/op_halt.c
        src/opcodes/op_bitwise_or.c
        src/opcodes/op_bitwise_xor.c
        src/opcodes/op_bitwise_not.c
        src/opcodes/op_left_shift.c
        src/opcodes/op_right_shift.c
        src/opcodes/op_logical_right_shift.c
        src/opcodes/op_floor_div.c
        src/opcodes/op_increment.c
        src/opcodes/op_decrement.c
        src/opcodes/op_in.c
        src/opcodes/op_call_method.c
        src/opcodes/op_pop_n_preserve_top.c
        src/opcodes/op_build_range.c
        src/opcodes/op_jump_if_false.c
        src/opcodes/op_jump.c
        src/opcodes/op_loop.c
        src/opcodes/op_pop_n.c
        src/opcodes/op_or.c
        src/opcodes/op_not.c
        src/opcodes/op_null_coalesce.c
        src/opcodes/op_instanceof.c
        deps/cargs/src/cargs.c
)

# Link math library for main executable
target_link_libraries(slate m)


# Tests executable (using Unity framework)
option(BUILD_TESTS "Build unit tests" ON)

if (BUILD_TESTS)
    # Download Unity if not present
    if (NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/devdeps/unity")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.c"
                "${CMAKE_CURRENT_SOURCE_DIR}/devdeps/unity/unity.c")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/devdeps/unity/unity.h")
        file(DOWNLOAD
                "https://raw.githubusercontent.com/ThrowTheSwitch/Unity/master/src/unity_internals.h"
                "${CMAKE_CURRENT_SOURCE_DIR}/devdeps/unity/unity_internals.h")
    endif ()

    include_directories(devdeps/unity)

    add_executable(slate_tests
            devdeps/unity/unity.c
            tests/test_main.c
            tests/test_lexer.c
            tests/test_parser.c
            tests/test_vm.c
            tests/test_conditionals.c
            tests/test_while_loops.c
            tests/test_for_loops.c
            tests/test_infinite_loops.c
            tests/test_builtins.c
            tests/test_buffer_class.c
            tests/test_integers.c
            tests/test_arithmetic.c
            tests/test_logical.c
            tests/test_break_statements.c
            tests/test_continue_statements.c
            tests/test_nested_loops.c
            tests/test_assignment.c
            tests/test_string.c
            tests/test_localdate.c
            tests/test_localtime.c
            tests/test_new_operators.c
            tests/test_functions.c
            deps/cargs/src/cargs.c
            src/lexer.c
            src/parser.c
            src/ast.c
            src/codegen.c
            src/vm.c
            src/value.c
            src/builtins.c
            src/library_impl.c
            src/datetime.c
            src/classes/String/factory.c
            src/classes/String/methods.c
            src/classes/String/builder.c
            src/classes/Array/methods.c
            src/classes/Array/functional.c
            src/classes/Array/class.c
            src/classes/Buffer/methods.c
            src/classes/Buffer/factory.c
            src/classes/Buffer/class.c
            src/classes/BufferBuilder/buffer_builder.c
            src/classes/BufferReader/buffer_reader.c
            src/classes/Range/range.c
            src/classes/Iterator/iterator.c
            src/classes/Value/value.c
            src/classes/Int/int.c
            src/opcodes/op_add.c
        src/opcodes/op_subtract.c
        src/opcodes/op_multiply.c
        src/opcodes/op_divide.c
        src/opcodes/op_mod.c
        src/opcodes/op_negate.c
        src/opcodes/op_equal.c
        src/opcodes/op_not_equal.c
        src/opcodes/op_less.c
        src/opcodes/op_less_equal.c
        src/opcodes/op_greater.c
        src/opcodes/op_greater_equal.c
        src/opcodes/op_return.c
        src/opcodes/op_get_local.c
        src/opcodes/op_set_local.c
        src/opcodes/op_define_global.c
        src/opcodes/op_set_global.c
        src/opcodes/op_build_object.c
        src/opcodes/op_power.c
        src/opcodes/op_build_array.c
        src/opcodes/op_and.c
        src/opcodes/op_bitwise_and.c
        src/opcodes/op_push_constant.c
        src/opcodes/op_get_global.c
        src/opcodes/op_get_property.c
        src/opcodes/op_call.c
        src/opcodes/op_closure.c
        src/opcodes/op_set_debug_location.c
        src/opcodes/op_push_null.c
        src/opcodes/op_push_undefined.c
        src/opcodes/op_push_true.c
        src/opcodes/op_push_false.c
        src/opcodes/op_pop.c
        src/opcodes/op_dup.c
        src/opcodes/op_set_result.c
        src/opcodes/op_clear_debug_location.c
        src/opcodes/op_halt.c
        src/opcodes/op_bitwise_or.c
        src/opcodes/op_bitwise_xor.c
        src/opcodes/op_bitwise_not.c
        src/opcodes/op_left_shift.c
        src/opcodes/op_right_shift.c
        src/opcodes/op_logical_right_shift.c
        src/opcodes/op_floor_div.c
        src/opcodes/op_increment.c
        src/opcodes/op_decrement.c
        src/opcodes/op_in.c
        src/opcodes/op_call_method.c
        src/opcodes/op_pop_n_preserve_top.c
        src/opcodes/op_build_range.c
        src/opcodes/op_jump_if_false.c
        src/opcodes/op_jump.c
        src/opcodes/op_loop.c
        src/opcodes/op_pop_n.c
        src/opcodes/op_or.c
        src/opcodes/op_not.c
        src/opcodes/op_null_coalesce.c
        src/opcodes/op_instanceof.c
    )

    # Link math library for tests executable
    target_link_libraries(slate_tests m)

    # Define Unity config for all test files
    target_compile_definitions(slate_tests PRIVATE UNITY_INCLUDE_CONFIG_H)

    enable_testing()
    add_test(NAME slate_tests COMMAND slate_tests)
endif ()
