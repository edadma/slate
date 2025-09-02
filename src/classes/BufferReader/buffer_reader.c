#include "buffer_reader.h"
#include "builtins.h"
#include "dynamic_buffer.h"
#include <stdint.h>
#include <limits.h>

// Global BufferReader class storage
value_t* global_buffer_reader_class = NULL;

// buffer_reader(buffer) - Create buffer reader
value_t builtin_buffer_reader(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "buffer_reader() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t buffer_val = args[0];
    if (buffer_val.type != VAL_BUFFER) {
        runtime_error(vm, "buffer_reader() requires a buffer argument, not %s", value_type_name(buffer_val.type));
    }

    db_reader reader = db_reader_new(buffer_val.as.buffer);
    return make_buffer_reader(reader);
}

// reader_read_uint8(reader) - Read uint8 from reader
value_t builtin_reader_read_uint8(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader_read_uint8() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error(vm, "reader_read_uint8() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 1)) {
        runtime_error(vm, "Cannot read uint8: not enough data remaining");
    }

    uint8_t value = db_read_uint8(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint16_le(reader) - Read uint16 in little-endian from reader
value_t builtin_reader_read_uint16_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader_read_uint16_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error(vm, "reader_read_uint16_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 2)) {
        runtime_error(vm, "Cannot read uint16: not enough data remaining");
    }

    uint16_t value = db_read_uint16_le(reader);
    return make_int32((int32_t)value);
}

// reader_read_uint32_le(reader) - Read uint32 in little-endian from reader
value_t builtin_reader_read_uint32_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader_read_uint32_le() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error(vm, "reader_read_uint32_le() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    db_reader reader = reader_val.as.reader;
    if (!db_reader_can_read(reader, 4)) {
        runtime_error(vm, "Cannot read uint32: not enough data remaining");
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
value_t builtin_reader_position(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader_position() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error(vm, "reader_position() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t pos = db_reader_position(reader_val.as.reader);
    return make_int32((int32_t)pos);
}

// reader_remaining(reader) - Get remaining bytes in reader
value_t builtin_reader_remaining(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "reader_remaining() takes exactly 1 argument (%d given)", arg_count);
    }

    value_t reader_val = args[0];
    if (reader_val.type != VAL_BUFFER_READER) {
        runtime_error(vm, "reader_remaining() requires a buffer reader, not %s", value_type_name(reader_val.type));
    }

    size_t remaining = db_reader_remaining(reader_val.as.reader);
    return make_int32((int32_t)remaining);
}

// ============================================================================
// BufferReader Class Implementation
// ============================================================================

// BufferReader factory function
value_t buffer_reader_factory(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader() requires 1 argument: buffer");
    }
    
    value_t buffer_val = args[0];
    if (buffer_val.type != VAL_BUFFER) {
        runtime_error(vm, "BufferReader() requires a buffer argument, not %s", value_type_name(buffer_val.type));
    }

    db_reader reader = db_reader_new(buffer_val.as.buffer);
    value_t reader_obj = make_buffer_reader(reader);
    
    // Set the class for the reader object
    if (global_buffer_reader_class) {
        reader_obj.class = global_buffer_reader_class;
    }
    
    return reader_obj;
}

// BufferReader instance method: readUint8()
value_t builtin_buffer_reader_read_uint8(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader.readUint8() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_BUFFER_READER) {
        runtime_error(vm, "BufferReader.readUint8() can only be called on BufferReader objects");
        return make_null();
    }
    
    db_reader reader = args[0].as.reader;
    if (!db_reader_can_read(reader, 1)) {
        runtime_error(vm, "Cannot read uint8: not enough data remaining");
        return make_null();
    }

    uint8_t value = db_read_uint8(reader);
    return make_int32((int32_t)value);
}

// BufferReader instance method: readUint16LE()
value_t builtin_buffer_reader_read_uint16_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader.readUint16LE() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_BUFFER_READER) {
        runtime_error(vm, "BufferReader.readUint16LE() can only be called on BufferReader objects");
        return make_null();
    }
    
    db_reader reader = args[0].as.reader;
    if (!db_reader_can_read(reader, 2)) {
        runtime_error(vm, "Cannot read uint16: not enough data remaining");
        return make_null();
    }

    uint16_t value = db_read_uint16_le(reader);
    return make_int32((int32_t)value);
}

// BufferReader instance method: readUint32LE()
value_t builtin_buffer_reader_read_uint32_le(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader.readUint32LE() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_BUFFER_READER) {
        runtime_error(vm, "BufferReader.readUint32LE() can only be called on BufferReader objects");
        return make_null();
    }
    
    db_reader reader = args[0].as.reader;
    if (!db_reader_can_read(reader, 4)) {
        runtime_error(vm, "Cannot read uint32: not enough data remaining");
        return make_null();
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

// BufferReader instance method: position()
value_t builtin_buffer_reader_position(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader.position() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_BUFFER_READER) {
        runtime_error(vm, "BufferReader.position() can only be called on BufferReader objects");
        return make_null();
    }
    
    size_t pos = db_reader_position(args[0].as.reader);
    return make_int32((int32_t)pos);
}

// BufferReader instance method: remaining()
value_t builtin_buffer_reader_remaining(vm_t* vm, int arg_count, value_t* args) {
    if (arg_count != 1) {
        runtime_error(vm, "BufferReader.remaining() takes 1 argument (self)");
        return make_null();
    }
    
    if (args[0].type != VAL_BUFFER_READER) {
        runtime_error(vm, "BufferReader.remaining() can only be called on BufferReader objects");
        return make_null();
    }
    
    size_t remaining = db_reader_remaining(args[0].as.reader);
    return make_int32((int32_t)remaining);
}

// BufferReader class initialization
void buffer_reader_class_init(vm_t* vm) {
    // Create the BufferReader class with its prototype
    do_object buffer_reader_proto = do_create(NULL);

    // Add methods to BufferReader prototype
    value_t read_uint8_method = make_native(builtin_buffer_reader_read_uint8);
    do_set(buffer_reader_proto, "readUint8", &read_uint8_method, sizeof(value_t));

    value_t read_uint16_le_method = make_native(builtin_buffer_reader_read_uint16_le);
    do_set(buffer_reader_proto, "readUint16LE", &read_uint16_le_method, sizeof(value_t));

    value_t read_uint32_le_method = make_native(builtin_buffer_reader_read_uint32_le);
    do_set(buffer_reader_proto, "readUint32LE", &read_uint32_le_method, sizeof(value_t));

    value_t position_method = make_native(builtin_buffer_reader_position);
    do_set(buffer_reader_proto, "position", &position_method, sizeof(value_t));

    value_t remaining_method = make_native(builtin_buffer_reader_remaining);
    do_set(buffer_reader_proto, "remaining", &remaining_method, sizeof(value_t));

    // Create the BufferReader class
    value_t buffer_reader_class = make_class("BufferReader", buffer_reader_proto);
    
    // Set the factory function to allow BufferReader(buffer)
    buffer_reader_class.as.class->factory = buffer_reader_factory;
    
    // Store in globals
    do_set(vm->globals, "BufferReader", &buffer_reader_class, sizeof(value_t));

    // Store a global reference for use in make_buffer_reader
    static value_t buffer_reader_class_storage;
    buffer_reader_class_storage = vm_retain(buffer_reader_class);
    global_buffer_reader_class = &buffer_reader_class_storage;
}