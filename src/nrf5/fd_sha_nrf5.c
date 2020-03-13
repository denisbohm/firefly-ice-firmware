#include "fd_sha.h"

#include "fd_log.h"

#include "fd_nrf5.h"

#include <string.h>

bool fd_sha1_is_equal(const uint8_t *a, const uint8_t *b) {
    for (int i = 0; i < FD_SHA_HASH_SIZE; ++i) {
        if (a[i] != b[i]) {
            return false;
        }
    }
    return true;
}

void fd_sha_source(uint32_t address, uint8_t *data, uint32_t length) {
    memcpy(data, (void *)address, length);
}

#if !NRF_MODULE_ENABLED(NRF_CRYPTO_BACKEND_CC310)

void fd_sha_initialize(void) {
}

void fd_sha1(fd_sha_source_t source, uint32_t address, uint32_t length, uint8_t *hash) {
}

#else

void fd_sha_initialize(void) {
}

void fd_sha1(fd_sha_source_t source, uint32_t address, uint32_t length, uint8_t *hash) {
    NVIC_EnableIRQ(CRYPTOCELL_IRQn);
    NRF_CRYPTOCELL->ENABLE = 1;
    SA_SilibRetCode_t ret = SaSi_LibInit();
    fd_log_assert(ret == SA_SILIB_RET_OK);

    CRYS_HASHUserContext_t context;
    ret = CRYS_HASH_Init(&context, CRYS_HASH_SHA1_mode);
    fd_log_assert(ret == CRYS_OK);
    uint8_t data[FD_SHA_BLOCK_SIZE];
    uint32_t remaining = length;
    while (remaining > 0) {
        uint32_t n = FD_SHA_BLOCK_SIZE;
        if (remaining < n) {
            n = remaining;
        }
        (*source)(address, data, n);
        ret = CRYS_HASH_Update(&context, data, n);
        fd_log_assert(ret == CRYS_OK);
        remaining -= n;
        address += n;
    }
    CRYS_HASH_Result_t result;
    ret = CRYS_HASH_Finish(&context, result);
    fd_log_assert(ret == CRYS_OK);
    memcpy(hash, result, FD_SHA_HASH_SIZE);

    SaSi_LibFini();
    NVIC_DisableIRQ(CRYPTOCELL_IRQn);
    NRF_CRYPTOCELL->ENABLE = 0;
}

#endif
