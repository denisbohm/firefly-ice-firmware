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
double fd_binary_unpack_double64(uint8_t *buffer);
fd_time_t fd_binary_unpack_time64(uint8_t *buffer);
void fd_binary_unpack_utf8(uint8_t *buffer, uint8_t **data, uint16_t *length);

void fd_binary_pack_uint8(uint8_t *buffer, uint8_t value);
void fd_binary_pack_uint16(uint8_t *buffer, uint16_t value);
void fd_binary_pack_uint24(uint8_t *buffer, uint32_t value);
void fd_binary_pack_uint32(uint8_t *buffer, uint32_t value);
void fd_binary_pack_uint64(uint8_t *buffer, uint64_t value);
void fd_binary_pack_float16(uint8_t *buffer, float value);
void fd_binary_pack_float32(uint8_t *buffer, float value);
void fd_binary_pack_double64(uint8_t *buffer, double value);
void fd_binary_pack_time64(uint8_t *buffer, fd_time_t value);
void fd_binary_pack_utf8(uint8_t *buffer, uint8_t *data, uint16_t length);

typedef struct {
    uint8_t *buffer;
    uint32_t size;
    uint32_t put_index;
    uint32_t get_index;
    uint32_t flags;
} fd_binary_t;

#define FD_BINARY_FLAG_OVERFLOW 0x00000001
#define FD_BINARY_FLAG_INVALID_REPRESENTATION 0x00000002
#define FD_BINARY_FLAG_OUT_OF_BOUNDS 0x00000004

typedef struct {
    uint64_t length;
    uint8_t *data;
} fd_binary_string_t;

void fd_binary_initialize(fd_binary_t *binary, uint8_t *buffer, uint32_t size);

uint32_t fd_binary_remaining_length(fd_binary_t *binary);

bool fd_binary_get_check(fd_binary_t *binary, uint32_t length);
void fd_binary_get_bytes(fd_binary_t *binary, uint8_t *data, uint32_t length);
uint8_t fd_binary_get_uint8(fd_binary_t *binary);
uint16_t fd_binary_get_uint16(fd_binary_t *binary);
uint32_t fd_binary_get_uint24(fd_binary_t *binary);
uint32_t fd_binary_get_uint32(fd_binary_t *binary);
uint64_t fd_binary_get_uint64(fd_binary_t *binary);
float fd_binary_get_float16(fd_binary_t *binary);
float fd_binary_get_float32(fd_binary_t *binary);
double fd_binary_get_double64(fd_binary_t *binary);
fd_time_t fd_binary_get_time64(fd_binary_t *binary);
uint64_t fd_binary_get_varuint(fd_binary_t *binary);
int64_t fd_binary_get_varint(fd_binary_t *binary);
fd_binary_string_t fd_binary_get_string(fd_binary_t *binary);

bool fd_binary_put_check(fd_binary_t *binary, uint32_t length);
void fd_binary_put_bytes(fd_binary_t *binary, const uint8_t *data, uint32_t length);
void fd_binary_put_uint8(fd_binary_t *binary, uint8_t value);
void fd_binary_put_uint16(fd_binary_t *binary, uint16_t value);
void fd_binary_put_uint24(fd_binary_t *binary, uint32_t value);
void fd_binary_put_uint32(fd_binary_t *binary, uint32_t value);
void fd_binary_put_uint64(fd_binary_t *binary, uint64_t value);
void fd_binary_put_float16(fd_binary_t *binary, float value);
void fd_binary_put_float32(fd_binary_t *binary, float value);
void fd_binary_put_double64(fd_binary_t *binary, double value);
void fd_binary_put_time64(fd_binary_t *binary, fd_time_t value);
void fd_binary_put_varuint(fd_binary_t *binary, uint64_t value);
void fd_binary_put_varint(fd_binary_t *binary, int64_t value);
void fd_binary_put_string(fd_binary_t *binary, const char *string);

#endif