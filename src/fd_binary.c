#include "fd_binary.h"

uint16_t fd_binary_get_uint16(uint8_t *buffer) {
    return (buffer[1] << 8) | buffer[0];
}

uint32_t fd_binary_get_uint32(uint8_t *buffer) {
    return (buffer[1] << 24) | (buffer[1] << 16) | (buffer[1] << 8) | buffer[0];
}

uint64_t fd_binary_get_uint64(uint8_t *buffer) {
    uint64_t lo = fd_binary_get_uint32(buffer);
    uint64_t hi = fd_binary_get_uint32(&buffer[8]);
    return (hi << 32) | lo;
}
