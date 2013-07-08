#ifndef FD_AES_H
#define FD_AES_H

#include <stdint.h>

typedef struct {
    uint32_t prev[4];
} fd_aes_decrypt_t;

void fd_aes_decrypt_start(fd_aes_decrypt_t *decrypt, const uint8_t *key, const uint8_t *iv);
void fd_aes_decrypt_blocks(fd_aes_decrypt_t *decrypt, uint8_t *in, uint8_t *out, uint32_t length);
void fd_aes_decrypt_stop(fd_aes_decrypt_t *decrypt);

#endif