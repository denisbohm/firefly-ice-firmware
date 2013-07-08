#ifndef FD_SHA_H
#define FD_SHA_H

#include <stdbool.h>
#include <stdint.h>

typedef void (*fd_sha1_source_t)(uint32_t address, uint8_t *data, uint32_t length);

void fd_sha1(fd_sha1_source_t source, uint32_t address, uint32_t length, uint8_t *hash);

bool fd_sha1_is_equal(uint8_t *a, uint8_t *b);

#endif