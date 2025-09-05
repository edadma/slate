cmake_minimum_required(VERSION 3.10)
project(slate C)

set(CMAKE_C_STANDARD 11)

# Build configuration options
option(DEFAULT_FLOAT32 "Use float32 as default floating point type" OFF)
option(MCU_MODE "Enable MCU optimizations (implies DEFAULT_FLOAT32)" OFF)

# Timezone configuration options
option(FULL_TIMEZONE "Use system timezone database for full IANA timezone support" ON)
option(EMBEDDED_TIMEZONE "Use embedded timezone subset for MCU compatibility" OFF)
set(EMBEDDED_TIMEZONE_SET "canada" CACHE STRING "Timezone set for embedded builds: canada")

# Override FULL_TIMEZONE if EMBEDDED_TIMEZONE is explicitly enabled
if(EMBEDDED_TIMEZONE)
    set(FULL_TIMEZONE OFF)
endif()

# Validation for timezone options (after override)
if(FULL_TIMEZONE AND EMBEDDED_TIMEZONE)
    message(FATAL_ERROR "Cannot enable both FULL_TIMEZONE and EMBEDDED_TIMEZONE. Choose one or neither (for UTC-only mode).")
endif()

# If MCU mode is enabled, automatically enable float32 as default
if(MCU_MODE)
    set(DEFAULT_FLOAT32 ON CACHE BOOL "Use float32 as default floating point type" FORCE)
endif()

# Generate configuration header
configure_file(config.h.in ${CMAKE_BINARY_DIR}/config.h)

# Include directories
include_directories(include)
include_directories(${CMAKE_BINARY_DIR})
include_directories(src/parser)
include_directories(src/vm)
include_directories(src/codegen)
include_directories(deps/cargs/include)
include_directories(deps/dynamic_string.h)
include_directories(deps/dynamic_array.h)
include_directories(deps/dynamic_object.h)
include_directories(deps/dynamic_int.h)
include_directories(deps/dynamic_buffer.h)
include_directories(deps/stb_ds.h)
include_directories(src/classes/String)
include_directories(src/classes/Boolean)
include_directories(src/classes/Array)
include_directories(src/classes/Buffer)
include_directories(src/classes/BufferBuilder)
include_directories(src/classes/BufferReader)
include_directories(src/classes/Range)
include_directories(src/classes/Iterator)
include_directories(src/classes/Value)
include_directories(src/classes/Number)
include_directories(src/classes/Int)
include_directories(src/classes/Float)
include_directories(src/classes/LocalDate)
include_directories(src/classes/LocalTime)
include_directories(src/classes/LocalDateTime)
include_directories(src/classes/Zone)
include_directories(src/classes/Date)
include_directories(src/classes/Instant)
include_directories(src/classes/Null)
include_directories(src/classes/Object)
include_directories(src/classes/ADT)

# Main executable
add_executable(slate
        src/main.c
        src/lexer.c
        src/runtime_error.c
        src/parser/parser.c
        src/parser/declarations.c
        src/parser/statements.c
        src/parser/expressions.c
        src/ast.c
        src/codegen/debug.c
        src/codegen/chunk.c
        src/codegen/lifecycle.c
        src/codegen/functions.c
        src/codegen/compiler.c
        src/codegen/expressions.c
        src/codegen/literals.c
        src/codegen/operators.c
        src/codegen/statements.c
        src/codegen/control_flow.c
        src/codegen/utilities.c
        src/codegen/scope.c
        src/codegen/error.c
        src/codegen/disassembler.c
        src/vm/lifecycle.c
        src/vm/stack.c
        src/vm/execution.c
        src/vm/values.c
        src/utilities.c
        src/vm/constants.c
        src/vm/functions.c
        src/vm/opcodes.c
        src/vm/core.c
        src/vm/iterators.c
        src/vm/memory.c
        src/vm/debug.c
        src/value.c
        src/builtins.c
        src/library_impl.c
        src/module.c
        src/line_editor.c
        src/datetime.c
        src/timezone.c
        src/classes/String/factory.c
        src/classes/String/methods.c
        src/classes/Boolean/factory.c
        src/classes/Boolean/methods.c
        src/classes/StringBuilder/string_builder.c
        src/classes/Array/methods.c
        src/classes/Array/functional.c
        src/classes/Array/class.c
        src/classes/Buffer/methods.c
        src/classes/Buffer/factory.c
        src/classes/Buffer/class.c
        src/classes/BufferBuilder/factory.c
        src/classes/BufferBuilder/methods.c
        src/classes/BufferBuilder/class.c
        src/classes/BufferReader/buffer_reader.c
        src/classes/Range/range.c
        src/classes/Iterator/iterator.c
        src/classes/Value/value.c
        src/classes/Number/class.c
        src/classes/Number/methods.c
        src/classes/Int/int.c
        src/classes/Float/class.c
        src/classes/Float/factory.c
        src/classes/Float/methods.c
        src/classes/LocalDate/factory.c
        src/classes/LocalDate/methods.c
        src/classes/LocalDate/class.c
        src/classes/LocalTime/local_time.c
        src/classes/LocalDateTime/factory.c
        src/classes/LocalDateTime/methods.c
        src/classes/LocalDateTime/class.c
        src/classes/Zone/factory.c
        src/classes/Zone/methods.c
        src/classes/Zone/class.c
        src/classes/Date/factory.c
        src/classes/Date/methods.c
        src/classes/Date/class.c
        src/classes/Instant/factory.c
        src/classes/Instant/methods.c
        src/classes/Instant/class.c
        src/classes/Null/methods.c
        src/classes/Null/class.c
        src/classes/Object/methods.c
        src/classes/Object/class.c
        src/classes/ADT/adt_methods.c
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
        src/opcodes/op_get_upvalue.c
        src/opcodes/op_set_upvalue.c
        src/opcodes/op_build_object.c
        src/opcodes/op_power.c
        src/opcodes/op_build_array.c
        src/opcodes/op_set_index.c
        src/opcodes/op_bitwise_and.c
        src/opcodes/op_push_constant.c
        src/opcodes/op_get_global.c
        src/opcodes/op_get_property.c
        src/opcodes/op_set_property.c
        src/opcodes/op_call.c
        src/opcodes/op_closure.c
        src/opcodes/op_set_debug_location.c
        src/opcodes/op_push_null.c
        src/opcodes/op_push_undefined.c
        src/opcodes/op_push_true.c
        src/opcodes/op_push_false.c
        src/opcodes/op_pop.c
        src/opcodes/op_dup.c
        src/opcodes/op_swap.c
        src/opcodes/op_nip.c
        src/opcodes/op_rot.c
        src/opcodes/op_over.c
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
        src/opcodes/op_jump_if_true.c
        src/opcodes/op_jump.c
        src/opcodes/op_loop.c
        src/opcodes/op_pop_n.c
        src/opcodes/op_not.c
        src/opcodes/op_null_coalesce.c
        src/opcodes/op_instanceof.c
        src/opcodes/op_import_module.c
        src/opcodes/op_get_export.c
        src/opcodes/op_call_adt_base_class.c
        src/opcodes/op_create_adt_constructor.c
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
            tests/test_helpers.c
            tests/test_lexer.c
            src/runtime_error.c
            tests/test_parser.c
            tests/test_vm.c
            tests/test_conditionals.c
            tests/test_while_loops.c
            tests/test_for_loops.c
            tests/test_infinite_loops.c
            tests/test_builtins.c
            tests/test_buffer_class.c
            tests/test_buffer_builder_class.c
            tests/test_class_array.c
            tests/test_class_range.c
            tests/test_stepped_ranges.c
            tests/test_class_iterator.c
            tests/test_class_object.c
            tests/test_class_int.c
            tests/test_arithmetic.c
            tests/test_logical.c
            tests/test_break_statements.c
            tests/test_continue_statements.c
            tests/test_nested_loops.c
            tests/test_assignment.c
            tests/test_class_string.c
            tests/test_class_localdate.c
            tests/test_class_localtime.c
            tests/test_class_local_date_time.c
            tests/test_class_instant.c
            tests/test_class_date.c
            tests/test_new_operators.c
            tests/test_functions.c
            tests/test_variable_shadowing.c
            tests/test_immutable_variables.c
            tests/test_optional_chaining.c
            tests/test_match.c
            tests/test_data_types.c
            deps/cargs/src/cargs.c
            src/lexer.c
            src/parser/parser.c
        src/parser/declarations.c
        src/parser/statements.c
        src/parser/expressions.c
            src/ast.c
            src/codegen/debug.c
        src/codegen/chunk.c
        src/codegen/lifecycle.c
        src/codegen/functions.c
        src/codegen/compiler.c
        src/codegen/expressions.c
        src/codegen/literals.c
        src/codegen/operators.c
        src/codegen/statements.c
        src/codegen/control_flow.c
        src/codegen/utilities.c
        src/codegen/scope.c
        src/codegen/error.c
        src/codegen/disassembler.c
            src/vm/lifecycle.c
        src/vm/stack.c
        src/vm/execution.c
        src/vm/values.c
        src/utilities.c
        src/vm/constants.c
        src/vm/functions.c
        src/vm/opcodes.c
        src/vm/core.c
        src/vm/iterators.c
        src/vm/memory.c
        src/vm/debug.c
            src/value.c
            src/builtins.c
            src/library_impl.c
            src/module.c
            src/datetime.c
            src/timezone.c
                src/classes/String/factory.c
            src/classes/String/methods.c
            src/classes/Boolean/factory.c
            src/classes/Boolean/methods.c
            src/classes/StringBuilder/string_builder.c
            src/classes/Array/methods.c
            src/classes/Array/functional.c
            src/classes/Array/class.c
            src/classes/Buffer/methods.c
            src/classes/Buffer/factory.c
            src/classes/Buffer/class.c
            src/classes/BufferBuilder/factory.c
        src/classes/BufferBuilder/methods.c
        src/classes/BufferBuilder/class.c
            src/classes/BufferReader/buffer_reader.c
            src/classes/Range/range.c
            src/classes/Iterator/iterator.c
            src/classes/Value/value.c
            src/classes/Number/class.c
            src/classes/Number/methods.c
            src/classes/Int/int.c
            src/classes/Float/class.c
            src/classes/Float/factory.c
            src/classes/Float/methods.c
        src/classes/LocalDate/factory.c
        src/classes/LocalDate/methods.c
        src/classes/LocalDate/class.c
        src/classes/LocalTime/local_time.c
        src/classes/LocalDateTime/factory.c
        src/classes/LocalDateTime/methods.c
        src/classes/LocalDateTime/class.c
        src/classes/Zone/factory.c
        src/classes/Zone/methods.c
        src/classes/Zone/class.c
        src/classes/Date/factory.c
        src/classes/Date/methods.c
        src/classes/Date/class.c
        src/classes/Instant/factory.c
        src/classes/Instant/methods.c
        src/classes/Instant/class.c
        src/classes/Null/methods.c
        src/classes/Null/class.c
        src/classes/Object/methods.c
        src/classes/Object/class.c
        src/classes/ADT/adt_methods.c
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
        src/opcodes/op_get_upvalue.c
        src/opcodes/op_set_upvalue.c
        src/opcodes/op_build_object.c
        src/opcodes/op_power.c
        src/opcodes/op_build_array.c
        src/opcodes/op_set_index.c
        src/opcodes/op_bitwise_and.c
        src/opcodes/op_push_constant.c
        src/opcodes/op_get_global.c
        src/opcodes/op_get_property.c
        src/opcodes/op_set_property.c
        src/opcodes/op_call.c
        src/opcodes/op_closure.c
        src/opcodes/op_set_debug_location.c
        src/opcodes/op_push_null.c
        src/opcodes/op_push_undefined.c
        src/opcodes/op_push_true.c
        src/opcodes/op_push_false.c
        src/opcodes/op_pop.c
        src/opcodes/op_dup.c
        src/opcodes/op_swap.c
        src/opcodes/op_nip.c
        src/opcodes/op_rot.c
        src/opcodes/op_over.c
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
        src/opcodes/op_jump_if_true.c
        src/opcodes/op_jump.c
        src/opcodes/op_loop.c
        src/opcodes/op_pop_n.c
        src/opcodes/op_not.c
        src/opcodes/op_null_coalesce.c
        src/opcodes/op_instanceof.c
        src/opcodes/op_import_module.c
        src/opcodes/op_get_export.c
        src/opcodes/op_call_adt_base_class.c
        src/opcodes/op_create_adt_constructor.c
    )

    # Link math library for tests executable
    target_link_libraries(slate_tests m)

    # Define Unity config for all test files
    target_compile_definitions(slate_tests PRIVATE UNITY_INCLUDE_CONFIG_H)

    enable_testing()
    add_test(NAME slate_tests COMMAND slate_tests)
endif ()
