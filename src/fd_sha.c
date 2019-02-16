#include "fd_sha.h"

#include "sha.h"

#include <string.h>

void fd_sha_initialize(void) {
}

void fd_sha_source(FD_SHA_POINTER_UINT_TYPE address, uint8_t *data, uint32_t length) {
    memcpy(data, (void *)address, length);
}

void fd_sha1(fd_sha_source_t source, FD_SHA_POINTER_UINT_TYPE address, uint32_t length, uint8_t *hash) {
    SHA_CTX context;
    SHA1_Init(&context);
    sha1_byte data[SHA1_BLOCK_LENGTH];
    uint32_t remaining = length;
    while (remaining > 0) {
        uint32_t n = SHA1_BLOCK_LENGTH;
        if (remaining < n) {
            n = remaining;
        }
        (*source)(address, data, n);
        SHA1_Update(&context, data, n);
        remaining -= n;
        address += n;
    }
    SHA1_Final(hash, &context);
}

bool fd_sha1_is_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < FD_SHA_HASH_SIZE; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

