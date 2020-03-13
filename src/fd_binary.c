#include "fd_binary.h"
#include "fd_ieee754.h"
#include "fd_log.h"

#include <string.h>

uint8_t fd_binary_unpack_uint8(uint8_t *buffer) {
    return buffer[0];
}

uint16_t fd_binary_unpack_uint16(uint8_t *buffer) {
    return (buffer[1] << 8) | buffer[0];
}

uint32_t fd_binary_unpack_uint24(uint8_t *buffer) {
    return (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

uint32_t fd_binary_unpack_uint32(uint8_t *buffer) {
    return (buffer[3] << 24) | (buffer[2] << 16) | (buffer[1] << 8) | buffer[0];
}

uint64_t fd_binary_unpack_uint64(uint8_t *buffer) {
    uint64_t lo = fd_binary_unpack_uint32(buffer);
    uint64_t hi = fd_binary_unpack_uint32(&buffer[8]);
    return (hi << 32) | lo;
}

float fd_binary_unpack_float16(uint8_t *buffer) {
    uint16_t value = fd_binary_unpack_uint16(buffer);
    return fd_ieee754_uint16_to_float(value);
}

typedef union {
    uint32_t as_uint32;
    float as_float32;
} fd_int32_float32_t;

float fd_binary_unpack_float32(uint8_t *buffer) {
    fd_int32_float32_t u;
    u.as_uint32 = fd_binary_unpack_uint32(buffer);
    return u.as_float32;
}

typedef union {
    uint64_t as_uint64;
    double as_double64;
} fd_int64_double64_t;

double fd_binary_unpack_double64(uint8_t *buffer) {
    fd_int64_double64_t u;
    u.as_uint64 = fd_binary_unpack_uint64(buffer);
    return u.as_double64;
}

fd_time_t fd_binary_unpack_time64(uint8_t *buffer) {
    fd_time_t time;
    time.seconds = fd_binary_unpack_uint32(buffer);
    time.microseconds = fd_binary_unpack_uint32(&buffer[4]);
    return time;
}

void fd_binary_pack_uint8(uint8_t *buffer, uint8_t value) {
    buffer[0] = value;
}

void fd_binary_pack_uint16(uint8_t *buffer, uint16_t value) {
    buffer[0] = value;
    buffer[1] = value >> 8;
}

void fd_binary_pack_uint24(uint8_t *buffer, uint32_t value) {
    buffer[0] = value;
    buffer[1] = value >> 8;
    buffer[2] = value >> 16;
}

void fd_binary_pack_uint32(uint8_t *buffer, uint32_t value) {
    buffer[0] = value;
    buffer[1] = value >> 8;
    buffer[2] = value >> 16;
    buffer[3] = value >> 24;
}

void fd_binary_pack_uint64(uint8_t *buffer, uint64_t value) {
    fd_binary_pack_uint32(buffer, (uint32_t)value);
    fd_binary_pack_uint32(&buffer[4], (uint32_t)(value >> 32));
}

void fd_binary_pack_float16(uint8_t *buffer, float value) {
    uint16_t iv = fd_ieee754_float_to_uint16(value);
    fd_binary_pack_uint16(buffer, iv);
}

void fd_binary_pack_float32(uint8_t *buffer, float value) {
    fd_int32_float32_t u;
    u.as_float32 = value;
    fd_binary_pack_uint32(buffer, u.as_uint32);
}

void fd_binary_pack_double64(uint8_t *buffer, double value) {
    fd_int64_double64_t u;
    u.as_double64 = value;
    fd_binary_pack_uint64(buffer, u.as_uint64);
}

void fd_binary_pack_time64(uint8_t *buffer, fd_time_t value) {
    fd_binary_pack_uint32(buffer, value.seconds);
    fd_binary_pack_uint32(&buffer[4], value.microseconds);
}

void fd_binary_initialize(fd_binary_t *binary, uint8_t *buffer, uint32_t size) {
    binary->buffer = buffer;
    binary->size = size;
    binary->put_index = 0;
    binary->get_index = 0;
    binary->flags = 0;
}

void fd_binary_reset(fd_binary_t *binary) {
    binary->put_index = 0;
    binary->get_index = 0;
    binary->flags = 0;
}

void fd_binary_remove(fd_binary_t *binary, uint32_t index, uint32_t length) {
    uint8_t *buffer = binary->buffer;
    uint32_t amount = binary->put_index - length;
    memmove(&buffer[index], &buffer[index + length], amount);
    if (binary->get_index > (index + length)) {
        binary->get_index -= length;
    } else
    if (binary->get_index > index) {
        binary->get_index = index;
    }
    binary->put_index -= length;
}

uint32_t fd_binary_remaining_length(fd_binary_t *binary) {
    return binary->size - binary->put_index;
}

bool fd_binary_get_check(fd_binary_t *binary, uint32_t length) {
    if ((binary->get_index + length) <= binary->size) {
        return true;
    }
    binary->flags |= FD_BINARY_FLAG_OVERFLOW;
    fd_log_assert_fail("underflow");
    return false;
}

void fd_binary_get_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length) {
    if (!fd_binary_get_check(binary, length)) {
        memset(data, 0, length);
        return;
    }
    memcpy(data, &binary->buffer[binary->get_index], length);
    binary->get_index += length;
}

uint8_t fd_binary_get_uint8(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 1)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 1;
    return fd_binary_unpack_uint8(buffer);
}

uint16_t fd_binary_get_uint16(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 2)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 2;
    return fd_binary_unpack_uint16(buffer);
}

uint32_t fd_binary_get_uint24(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 3)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 3;
    return fd_binary_unpack_uint24(buffer);
}

uint32_t fd_binary_get_uint32(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 4)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 4;
    return fd_binary_unpack_uint32(buffer);
}

uint64_t fd_binary_get_uint64(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 8)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 8;
    return fd_binary_unpack_uint64(buffer);
}

float fd_binary_get_float16(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 2)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 2;
    return fd_binary_unpack_float16(buffer);
}

float fd_binary_get_float32(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 4)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 4;
    return fd_binary_unpack_float32(buffer);
}

double fd_binary_get_float64(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 8)) {
        return 0;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 8;
    return fd_binary_unpack_double64(buffer);
}

fd_time_t fd_binary_get_time64(fd_binary_t *binary) {
    if (!fd_binary_get_check(binary, 8)) {
        fd_time_t time = {.seconds = 0, .microseconds = 0};
        return time;
    }
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 8;
    return fd_binary_unpack_time64(buffer);
}

uint64_t fd_binary_get_varuint(fd_binary_t *binary) {
    uint64_t value = 0;
    uint32_t remaining = binary->size - binary->get_index;
    uint32_t index = 0;
    while (index < remaining) {
        uint64_t byte = binary->buffer[binary->get_index++];
        value |= (byte & 0x7f) << (index * 7);
        if ((byte & 0x80) == 0) {
            return value;
        }
        if ((value & 0xe000000000000000) != 0) {
            binary->flags |= FD_BINARY_FLAG_INVALID_REPRESENTATION;
            return 0;
        }
        index += 1;
    }
    binary->flags |= FD_BINARY_FLAG_OUT_OF_BOUNDS;
    return 0;
}

int64_t fd_binary_get_varint(fd_binary_t *binary) {
    uint64_t zig_zag = fd_binary_get_varuint(binary);
    uint64_t bit_pattern;
    if ((zig_zag & 0x0000000000000001) != 0) {
        bit_pattern = (zig_zag >> 1) ^ 0xffffffffffffffff;
    } else {
        bit_pattern = zig_zag >> 1;
    }
    return (int64_t)bit_pattern;
}

fd_binary_string_t fd_binary_get_string(fd_binary_t *binary) {
    uint64_t length = fd_binary_get_varuint(binary);
    uint32_t remaining = fd_binary_remaining_length(binary);
    if (remaining < length) {
        binary->flags |= FD_BINARY_FLAG_INVALID_REPRESENTATION;
        length = 0;
    }
    fd_binary_string_t string = {
        .length = length,
        .data = &binary->buffer[binary->get_index]
    };
    binary->get_index += length;
    return string;
}

bool fd_binary_put_check(fd_binary_t *binary, uint32_t length) {
    if ((binary->put_index + length) <= binary->size) {
        return true;
    }
    binary->flags |= FD_BINARY_FLAG_OVERFLOW;
    fd_log_assert_fail("overflow");
    return false;
}

void fd_binary_put_bytes(fd_binary_t *binary, const uint8_t *data, uint32_t length) {
    if (fd_binary_put_check(binary, length)) {
        memcpy(&binary->buffer[binary->put_index], data, length);
        binary->put_index += length;
    }
}

void fd_binary_put_uint8(fd_binary_t *binary, uint8_t value) {
    if (fd_binary_put_check(binary, sizeof(value))) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 1;
        fd_binary_pack_uint8(buffer, value);
    }
}

void fd_binary_put_uint16(fd_binary_t *binary, uint16_t value) {
    if (fd_binary_put_check(binary, sizeof(value))) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 2;
        fd_binary_pack_uint16(buffer, value);
    }
}

void fd_binary_put_uint24(fd_binary_t *binary, uint32_t value) {
    if (fd_binary_put_check(binary, 3)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 3;
        fd_binary_pack_uint24(buffer, value);
    }
}

void fd_binary_put_uint32(fd_binary_t *binary, uint32_t value) {
    if (fd_binary_put_check(binary, sizeof(value))) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 4;
        fd_binary_pack_uint32(buffer, value);
    }
}

void fd_binary_put_uint64(fd_binary_t *binary, uint64_t value) {
    if (fd_binary_put_check(binary, sizeof(value))) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 8;
        fd_binary_pack_uint64(buffer, value);
    }
}

void fd_binary_put_float16(fd_binary_t *binary, float value) {
    if (fd_binary_put_check(binary, 2)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 2;
        fd_binary_pack_float16(buffer, value);
    }
}

void fd_binary_put_float32(fd_binary_t *binary, float value) {
    if (fd_binary_put_check(binary, 4)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 4;
        fd_binary_pack_float32(buffer, value);
    }
}

void fd_binary_put_double64(fd_binary_t *binary, double value) {
    if (fd_binary_put_check(binary, 8)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 8;
        fd_binary_pack_double64(buffer, value);
    }
}

void fd_binary_put_time64(fd_binary_t *binary, fd_time_t value) {
    if (fd_binary_put_check(binary, 8)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 8;
        fd_binary_pack_time64(buffer, value);
    }
}

void fd_binary_put_varuint(fd_binary_t *binary, uint64_t value) {
    uint64_t remainder = value;
    while (remainder != 0) {
        if (remainder <= 0x7f) {
            break;
        }
        uint8_t byte = remainder | 0x80;
        fd_binary_put_uint8(binary, byte);
        remainder = remainder >> 7;
    }
    uint8_t byte = remainder;
    fd_binary_put_uint8(binary, byte);
}

void fd_binary_put_varint(fd_binary_t *binary, int64_t value) {
    uint64_t bit_pattern = (uint64_t)value;
    uint64_t zig_zag;
    if (value < 0) {
        zig_zag = (bit_pattern << 1) ^ 0xffffffffffffffff;
    } else {
        zig_zag = bit_pattern << 1;
    }
    fd_binary_put_varuint(binary, zig_zag);
}

void fd_binary_put_string(fd_binary_t *binary, const char *string) {
    uint32_t length = (uint32_t)strlen(string);
    fd_binary_put_varuint(binary, length);
    fd_binary_put_bytes(binary, (const uint8_t *)string, length);
}
