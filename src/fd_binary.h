#ifndef FD_BINARY_H
#define FD_BINARY_H

#include "fd_time.h"

#include <stdint.h>

uint8_t fd_binary_unpack_uint8(uint8_t *buffer);
uint16_t fd_binary_unpack_uint16(uint8_t *buffer);
uint32_t fd_binary_unpack_uint24(uint8_t *buffer);
uint32_t fd_binary_unpack_uint32(uint8_t *buffer);
uint64_t fd_binary_unpack_uint64(uint8_t *buffer);
float fd_binary_unpack_float16(uint8_t *buffer);
float fd_binary_unpack_float32(uint8_t *buffer);
fd_time_t fd_binary_unpack_time64(uint8_t *buffer);
void fd_binary_unpack_utf8(uint8_t *buffer, uint8_t **data, uint16_t *length);

void fd_binary_pack_uint8(uint8_t *buffer, uint8_t value);
void fd_binary_pack_uint16(uint8_t *buffer, uint16_t value);
void fd_binary_pack_uint24(uint8_t *buffer, uint32_t value);
void fd_binary_pack_uint32(uint8_t *buffer, uint32_t value);
void fd_binary_pack_uint64(uint8_t *buffer, uint64_t value);
void fd_binary_pack_float16(uint8_t *buffer, float value);
void fd_binary_pack_float32(uint8_t *buffer, float value);
void fd_binary_pack_time64(uint8_t *buffer, fd_time_t value);
void fd_binary_pack_utf8(uint8_t *buffer, uint8_t *data, uint16_t length);

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t put_index;
    uint32_t get_index;
} fd_binary_t;

void fd_binary_initialize(fd_binary_t *binary, uint8_t *buffer, uint32_t size);

uint32_t fd_binary_remaining_length(fd_binary_t *binary);

void fd_binary_get_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length);
uint8_t fd_binary_get_uint8(fd_binary_t *binary);
uint16_t fd_binary_get_uint16(fd_binary_t *binary);
uint32_t fd_binary_get_uint24(fd_binary_t *binary);
uint32_t fd_binary_get_uint32(fd_binary_t *binary);
uint64_t fd_binary_get_uint64(fd_binary_t *binary);
float fd_binary_get_float16(fd_binary_t *binary);
float fd_binary_get_float32(fd_binary_t *binary);
fd_time_t fd_binary_get_time64(fd_binary_t *binary);

void fd_binary_put_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length);
void fd_binary_put_uint8(fd_binary_t *binary, uint8_t value);
void fd_binary_put_uint16(fd_binary_t *binary, uint16_t value);
void fd_binary_put_uint24(fd_binary_t *binary, uint32_t value);
void fd_binary_put_uint32(fd_binary_t *binary, uint32_t value);
void fd_binary_put_uint64(fd_binary_t *binary, uint64_t value);
void fd_binary_put_float16(fd_binary_t *binary, float value);
void fd_binary_put_float32(fd_binary_t *binary, float value);
void fd_binary_put_time64(fd_binary_t *binary, fd_time_t value);

#endif