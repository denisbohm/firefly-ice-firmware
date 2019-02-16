#ifndef FD_SHA_H
#define FD_SHA_H

#include <stdbool.h>
#include <stdint.h>

#define FD_SHA_BLOCK_SIZE 64
#define FD_SHA_HASH_SIZE 20

#ifndef __LP64__
#define FD_SHA_POINTER_INT_TYPE int32_t
#define FD_SHA_POINTER_UINT_TYPE uint32_t
#else
#define FD_SHA_POINTER_INT_TYPE int64_t
#define FD_SHA_POINTER_UINT_TYPE uint64_t
#endif

typedef void (*fd_sha_source_t)(FD_SHA_POINTER_UINT_TYPE address, uint8_t *data, uint32_t length);

void fd_sha_source(FD_SHA_POINTER_UINT_TYPE address, uint8_t *data, uint32_t length);

void fd_sha_initialize(void);

void fd_sha1(fd_sha_source_t source, FD_SHA_POINTER_UINT_TYPE address, uint32_t length, uint8_t *hash);

bool fd_sha1_is_equal(const uint8_t *a, const uint8_t *b);

#endif
