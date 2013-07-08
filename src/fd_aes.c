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