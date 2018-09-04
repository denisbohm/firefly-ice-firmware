//
//  fdi_firmware_update.c
//  Atlas Device Simulator
//
//  Created by Denis Bohm on 7/21/18.
//  Copyright © 2018 Atlas Wearables. All rights reserved.
//

#include "fdi_firmware_update.h"

#include "fd_version.h"
#include "fd_hal_aes.h"
#include "fd_hal_external_flash.h"
#include "fd_hal_system.h"

#include <string.h>

typedef struct {
    fd_hal_aes_decrypt_t *decrypt;
} fdi_firmware_update_context_t;

void fdi_firmware_update_reader(void *untyped_context, uint32_t address, uint32_t length, uint8_t *data) {
    fdi_firmware_update_context_t *context = (fdi_firmware_update_context_t *)untyped_context;
    fd_hal_external_flash_read(address, data, length);
    if (context->decrypt) {
        uint32_t offset = 0;
        uint8_t output[64];
        uint32_t remaining = length;
        while (remaining > 0) {
            uint32_t amount = remaining;
            if (amount > sizeof(output)) {
                amount = sizeof(output);
            }
            fd_hal_aes_decrypt_blocks(context->decrypt, &data[offset], output, amount);
            memcpy(&data[offset], output, amount);
            offset += amount;
        }
    }
}

void fdi_firmware_update_before(fdi_firmware_update_context_t *context, uint8_t area, uint8_t *iv) {
    static uint8_t key[FD_HAL_AES_KEY_SIZE];
    fd_hal_system_get_crypto_key(area, key);
    if (context->decrypt) {
        fd_hal_aes_decrypt_start(context->decrypt, key, iv);
    }
}

void fdi_firmware_update_after(fdi_firmware_update_context_t *context) {
    if (context->decrypt) {
        fd_hal_aes_decrypt_stop(context->decrypt);
    }
}
