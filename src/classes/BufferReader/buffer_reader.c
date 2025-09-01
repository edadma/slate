#include "buffer_reader.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include <stdint.h>
#include <limits.h>

// buffer_reader(buffer) - Create buffer reader
value_t builtin_buffer_reader(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("buffer_reader() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    if (buffer_val.type != VAL_BUFFER) {
        runtime_error("buffer_reader() requires a buffer argument, not %s", value_type_name(buffer_val.type));
    }

    db_reader reader = db_reader_new(buffer_val.as.buffer);
    return make_buffer_reader(reader);
}

// reader_read_uint8(reader) - Read uint8 from reader
value_t builtin_reader_read_uint8(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint8() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint8() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 1)) {
        runtime_error("Cannot read uint8: not enough data remaining");
    }

    uint8_t value = db_read_uint8(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint16_le(reader) - Read uint16 in little-endian from reader
value_t builtin_reader_read_uint16_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint16_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint16_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 2)) {
        runtime_error("Cannot read uint16: not enough data remaining");
    }

    uint16_t value = db_read_uint16_le(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint32_le(reader) - Read uint32 in little-endian from reader
value_t builtin_reader_read_uint32_le(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_read_uint32_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_read_uint32_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 4)) {
        runtime_error("Cannot read uint32: not enough data remaining");
    }

    uint32_t value = db_read_uint32_le(reader);

    // Check if value fits in int32_t range
    if (value <= INT32_MAX) {
        return make_int32((int32_t)value);
    } else {
        // Convert to bigint for large values
        di_int big = di_from_uint32(value);
        return make_bigint(big);
    }
}

// reader_position(reader) - Get reader position
value_t builtin_reader_position(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_position() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_position() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t pos = db_reader_position(reader_val.as.reader);
    return make_int32((int32_t)pos);
}

// reader_remaining(reader) - Get remaining bytes in reader
value_t builtin_reader_remaining(slate_vm* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error("reader_remaining() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error("reader_remaining() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t remaining = db_reader_remaining(reader_val.as.reader);
    return make_int32((int32_t)remaining);
}