#ifndef FD_BINARY_H
#define FD_BINARY_H

#include <stdint.h>

uint16_t fd_binary_get_uint16(uint8_t *buffer);
uint32_t fd_binary_get_uint32(uint8_t *buffer);
uint64_t fd_binary_get_uint64(uint8_t *buffer);

#endif