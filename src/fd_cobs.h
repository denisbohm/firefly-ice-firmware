#ifndef FD_COBS_H
#define FD_COBS_H

#include <stddef.h>
#include <stdint.h>

size_t fd_cobs_encode(const uint8_t *src_data, size_t src_length, uint8_t *dst_data, size_t dst_length);
size_t fd_cobs_decode(const uint8_t *src_data, size_t src_length, uint8_t *dst_data, size_t dst_length);

#endif
