#ifndef FD_HAL_AES_H
#define FD_HAL_AES_H

#include <stdint.h>

#define FD_HAL_AES_KEY_SIZE 16
#define FD_HAL_AES_IV_SIZE 16

typedef struct {
    uint32_t prev[32];
} fd_hal_aes_decrypt_t;

void fd_hal_aes_decrypt_start(fd_hal_aes_decrypt_t *decrypt, const uint8_t *key, const uint8_t *iv);
void fd_hal_aes_decrypt_blocks(fd_hal_aes_decrypt_t *decrypt, uint8_t *in, uint8_t *out, uint32_t length);
void fd_hal_aes_decrypt_stop(fd_hal_aes_decrypt_t *decrypt);

#endif