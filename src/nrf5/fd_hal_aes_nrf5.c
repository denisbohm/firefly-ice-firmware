#include "fd_hal_aes.h"

#include "fd_log.h"

#include "fd_nrf5.h"

#ifndef CRYPTOCELL_IRQn

static nrf_crypto_aes_context_t cbc_mac_ctx;

void fd_hal_aes_decrypt_start(fd_hal_aes_decrypt_t *decrypt __attribute__((unused)), const uint8_t *key, const uint8_t *iv) {
    nrf_crypto_aes_context_t *p_ctx = &cbc_mac_ctx;
    ret_code_t ret_val = nrf_crypto_aes_init(p_ctx, &g_nrf_crypto_aes_cbc_128_info, NRF_CRYPTO_DECRYPT);
    fd_log_assert(ret_val == NRF_SUCCESS);

    ret_val = nrf_crypto_aes_key_set(p_ctx, (uint8_t *)key);
    fd_log_assert(ret_val == NRF_SUCCESS);

    ret_val = nrf_crypto_aes_iv_set(p_ctx, (uint8_t *)iv);
    fd_log_assert(ret_val == NRF_SUCCESS);
}

void fd_hal_aes_decrypt_blocks(fd_hal_aes_decrypt_t *decrypt __attribute__((unused)), uint8_t *in, uint8_t *out, uint32_t length) {
    nrf_crypto_aes_context_t *p_ctx = &cbc_mac_ctx;
    ret_code_t ret_val = nrf_crypto_aes_update(p_ctx, in, length, out);
    fd_log_assert(ret_val == NRF_SUCCESS);
}

void fd_hal_aes_decrypt_stop(fd_hal_aes_decrypt_t *decrypt __attribute__((unused))) {
    nrf_crypto_aes_context_t *p_ctx = &cbc_mac_ctx;
    nrf_crypto_aes_uninit(p_ctx);
}

#else

void fd_hal_aes_decrypt_start(fd_hal_aes_decrypt_t *decrypt, const uint8_t *key, const uint8_t *iv) {
    NVIC_EnableIRQ(CRYPTOCELL_IRQn);
    NRF_CRYPTOCELL->ENABLE = 1;

    SA_SilibRetCode_t ret = SaSi_LibInit();
    fd_log_assert(ret == SA_SILIB_RET_OK);

    SaSiAesUserContext_t *context = (SaSiAesUserContext_t *)decrypt;
    ret = SaSi_AesInit(context, SASI_AES_MODE_CBC, SASI_AES_DECRYPT, SASI_AES_PADDING_NONE);
    fd_log_assert(ret == SA_SILIB_RET_OK);
    ret = SaSi_AesSetIv(context, (uint8_t *)iv);
    fd_log_assert(ret == SA_SILIB_RET_OK);
    SaSiAesUserKeyData_t keyData = {
        .pKey = (uint8_t *)key,
        .keySize = FD_HAL_AES_KEY_SIZE
    };
    ret = SaSi_AesSetKey(context, SASI_AES_USER_KEY, &keyData, sizeof(keyData) );
    fd_log_assert(ret == SA_SILIB_RET_OK);
}

void fd_hal_aes_decrypt_blocks(fd_hal_aes_decrypt_t *decrypt __attribute__((unused)), uint8_t *in, uint8_t *out, uint32_t length) {
    SaSiAesUserContext_t *context = (SaSiAesUserContext_t *)decrypt;
    uint32_t remaining = length;
    while (remaining > 0) {
        uint32_t n = remaining;
        if (n > SASI_AES_BLOCK_SIZE_IN_BYTES) {
            n = SASI_AES_BLOCK_SIZE_IN_BYTES;
        }
        SA_SilibRetCode_t ret = SaSi_AesBlock(context, in, n, out);
        fd_log_assert(ret == SA_SILIB_RET_OK);
        in += n;
        out += n;
        remaining -= n;
    }
}

void fd_hal_aes_decrypt_stop(fd_hal_aes_decrypt_t *decrypt __attribute__((unused))) {
    SaSiAesUserContext_t *context = (SaSiAesUserContext_t *)decrypt;
    uint8_t in[SASI_AES_BLOCK_SIZE_IN_BYTES];
    uint8_t out[SASI_AES_BLOCK_SIZE_IN_BYTES];
    size_t out_size = 0;
    SA_SilibRetCode_t ret = SaSi_AesFinish(context, out_size, in, out_size, out, &out_size);
    fd_log_assert(ret == SA_SILIB_RET_OK);
}

#endif