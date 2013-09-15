#include "fd_aes.h"

#include <em_aes.h>
#include <em_cmu.h>

void fd_aes_decrypt_start(fd_aes_decrypt_t *decrypt, const uint8_t *key, const uint8_t *iv) {
    CMU_ClockEnable(cmuClock_AES, true);

    const uint32_t *_key = (const uint32_t *)key;
    /* Load key into high key for key buffer usage */
    for (int i = 3; i >= 0; i--) {
        AES->KEYHA = __REV(_key[i]);
    }

    AES->CTRL = AES_CTRL_DECRYPT | AES_CTRL_KEYBUFEN | AES_CTRL_DATASTART;

    const uint32_t *_iv = (const uint32_t *)iv;
    for (int i = 0; i < 4; i++) {
        decrypt->prev[i] = _iv[i];
    }
}

#define AES_BLOCKSIZE 16

void fd_aes_decrypt_blocks(fd_aes_decrypt_t *decrypt, uint8_t *in, uint8_t *out, uint32_t length) {
    uint32_t *prev = decrypt->prev;
    uint32_t *_in = (uint32_t *)in;
    uint32_t *_out = (uint32_t *)out;
    uint32_t blocks = length / AES_BLOCKSIZE;
    while (blocks--) {
         /* Load data and trigger decryption */
        for (int i = 3; i >= 0; i--) {
            AES->DATA = __REV(_in[i]);
        }

        /* Wait for completion */
        while (AES->STATUS & AES_STATUS_RUNNING);

        /* In order to avoid additional buffer, we use HW directly for XOR and buffer */
        /* (Writing to XORDATA will not trigger encoding, triggering enabled on DATA.) */
        for (int i = 3; i >= 0; i--) {
            AES->XORDATA = __REV(prev[i]);
            prev[i] = _in[i];
        }
        _in += 4;

        /* Then fetch decrypted data, we have to do it in a separate loop */
         /* due to internal auto-shifting of words */
        for (int i = 3; i >= 0; i--) {
            _out[i] = __REV(AES->DATA);
        }
        _out += 4;
    }
}

void fd_aes_decrypt_stop(fd_aes_decrypt_t *decrypt __attribute__((unused))) {
    CMU_ClockEnable(cmuClock_AES, false);
}

void fd_aes_hash_start(fd_aes_hash_t *hash, const uint8_t *key, const uint8_t *iv) {
    CMU_ClockEnable(cmuClock_AES, true);

    const uint32_t *_key = (const uint32_t *)key;
    /* Load key into high key for key buffer usage */
    for (int i = 3; i >= 0; i--) {
        AES->KEYHA = __REV(_key[i]);
    }

    /* Enable encryption with auto start using XOR */
    AES->CTRL = AES_CTRL_KEYBUFEN | AES_CTRL_XORSTART;

    /* Load initialization vector, since writing to DATA, it will */
    /* not trigger encryption. */
    const uint32_t *_iv = (const uint32_t *)iv;
    for (int i = 3; i >= 0; i--) {
        AES->DATA = __REV(_iv[i]);
    }

    hash->out_index = 0;
}

void fd_aes_hash_blocks(fd_aes_hash_t *hash, uint8_t *in, uint32_t length) {
    uint32_t *_in = (uint32_t *)in;
    uint32_t blocks = length / AES_BLOCKSIZE;
    while (blocks--) {
         /* Load data and trigger decryption */
        for (int i = 3; i >= 0; i--) {
            AES->DATA = __REV(_in[i]);
        }
        _in += 4;

        /* Wait for completion */
        while (AES->STATUS & AES_STATUS_RUNNING);

        /* Save encrypted data */
        for (int i = 3; i >= 0; i--) {
            hash->out[hash->out_index] = __REV(AES->DATA);
            if (++hash->out_index >= 20) {
                hash->out_index = 0;
            }
        }
    }
}

void fd_aes_hash_stop(fd_aes_hash_t *hash, uint8_t *result) {
    /* The last 20 encrypted bytes are the hash */
    for (uint32_t i = 0; i < 20; ++i) {
        result[i] = hash->out[hash->out_index];
        if (++hash->out_index >= 20) {
            hash->out_index = 0;
        }
    }

    CMU_ClockEnable(cmuClock_AES, false);
}

static uint8_t defaultHashKeyBytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f};

static uint8_t defaultHashIVBytes[] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, 0x10, 0x11, 0x12, 0x13};

void fd_aes_hash_start_default(fd_aes_hash_t *hash) {
    fd_aes_hash_start(hash, defaultHashKeyBytes, defaultHashIVBytes);
}

void fd_aes_hash_default(fd_aes_source_t source, uint32_t address, uint32_t length, uint8_t *result) {
    fd_aes_hash_t hash;
    fd_aes_hash_start_default(&hash);
    uint8_t data[AES_BLOCKSIZE];
    uint32_t remaining = length;
    while (remaining > 0) {
        uint32_t n = AES_BLOCKSIZE;
        if (remaining < n) {
            n = remaining;
        }
        (*source)(address, data, n);
        fd_aes_hash_blocks(&hash, data, n);
        remaining -= n;
    }
    fd_aes_hash_stop(&hash, result);
}
