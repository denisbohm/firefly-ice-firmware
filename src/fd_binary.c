#include "fd_binary.h"

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
    fd_binary_pack_uint32(buffer, value);
    fd_binary_pack_uint32(&buffer[4], value >> 32);
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

void fd_binary_put_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length) {
    memcpy(data, &binary->buffer[binary->put_index], length);
    binary->put_index += length;
}

void fd_binary_put_uint8(fd_binary_t *binary, uint8_t value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 1;
    fd_binary_pack_uint8(buffer, value);
}

void fd_binary_put_uint16(fd_binary_t *binary, uint16_t value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 2;
    fd_binary_pack_uint16(buffer, value);
}

void fd_binary_put_uint32(fd_binary_t *binary, uint32_t value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 4;
    fd_binary_pack_uint32(buffer, value);
}

void fd_binary_put_uint64(fd_binary_t *binary, uint64_t value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 8;
    fd_binary_pack_uint64(buffer, value);
}

void fd_binary_put_float32(fd_binary_t *binary, float value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 4;
    fd_binary_pack_float32(buffer, value);
}

void fd_binary_put_time64(fd_binary_t *binary, fd_time_t value) {
    uint8_t *buffer = &binary->buffer[binary->put_index];
    binary->put_index += 8;
    fd_binary_pack_time64(buffer, value);
}