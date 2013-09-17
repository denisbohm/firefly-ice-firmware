#ifndef FD_AES_H
#define FD_AES_H

#include <stdint.h>

#define FD_AES_KEY_SIZE 16
#define FD_AES_IV_SIZE 16

typedef struct {
    uint32_t prev[4];
} fd_aes_decrypt_t;

void fd_aes_decrypt_start(fd_aes_decrypt_t *decrypt, const uint8_t *key, const uint8_t *iv);
void fd_aes_decrypt_blocks(fd_aes_decrypt_t *decrypt, uint8_t *in, uint8_t *out, uint32_t length);
void fd_aes_decrypt_stop(fd_aes_decrypt_t *decrypt);

typedef struct {
    uint8_t out[20];
    uint32_t out_index;
} fd_aes_hash_t;

void fd_aes_hash_start_default(fd_aes_hash_t *hash);
void fd_aes_hash_start(fd_aes_hash_t *hash, const uint8_t *key, const uint8_t *iv);
void fd_aes_hash_blocks(fd_aes_hash_t *hash, uint8_t *in, uint32_t length);
void fd_aes_hash_stop(fd_aes_hash_t *hash, uint8_t *result);

typedef void (*fd_aes_source_t)(uint32_t address, uint8_t *data, uint32_t length);

void fd_aes_hash_default(fd_aes_source_t source, uint32_t address, uint32_t length, uint8_t *result);

#endif