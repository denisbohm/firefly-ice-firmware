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

void fd_binary_pack_time64(uint8_t *buffer, fd_time_t value) {
    fd_binary_pack_uint32(buffer, value.seconds);
    fd_binary_pack_uint32(&buffer[4], value.microseconds);
}

void fd_binary_initialize(fd_binary_t *binary, uint8_t *buffer, uint32_t size) {
    binary->buffer = buffer;
    binary->size = size;
    binary->put_index = 0;
    binary->get_index = 0;
}

uint32_t fd_binary_remaining_length(fd_binary_t *binary) {
    return binary->size - binary->put_index;
}

void fd_binary_get_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length) {
    memcpy(data, &binary->buffer[binary->get_index], length);
    binary->get_index += length;
}

uint8_t fd_binary_get_uint8(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 1;
    return fd_binary_unpack_uint8(buffer);
}

uint16_t fd_binary_get_uint16(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 2;
    return fd_binary_unpack_uint16(buffer);
}

uint32_t fd_binary_get_uint32(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 4;
    return fd_binary_unpack_uint32(buffer);
}

uint64_t fd_binary_get_uint64(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 8;
    return fd_binary_unpack_uint64(buffer);
}

float fd_binary_get_float16(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 2;
    return fd_binary_unpack_float16(buffer);
}

float fd_binary_get_float32(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 4;
    return fd_binary_unpack_float32(buffer);
}

fd_time_t fd_binary_get_time64(fd_binary_t *binary) {
    uint8_t *buffer = &binary->buffer[binary->get_index];
    binary->get_index += 8;
    return fd_binary_unpack_time64(buffer);
}

bool fd_binary_put_check(fd_binary_t *binary, uint32_t length) {
    if ((binary->put_index + length) <= binary->size) {
        return true;
    }
    fd_log_assert_fail("");
    return false;
}

void fd_binary_put_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length) {
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

void fd_binary_put_time64(fd_binary_t *binary, fd_time_t value) {
    if (fd_binary_put_check(binary, 8)) {
        uint8_t *buffer = &binary->buffer[binary->put_index];
        binary->put_index += 8;
        fd_binary_pack_time64(buffer, value);
    }
}