#ifndef FD_PROVISION_H
#define FD_PROVISION_H

#include "fd_hal_aes.h"

#include <stdint.h>

// provisioning format:
// - uint16_t version
// - uint16_t flags;
// - uint8_t[16] AES key
// - map

typedef struct {
    uint16_t version;
    uint16_t flags;
    uint8_t key[FD_HAL_AES_KEY_SIZE];
    // map follows...
} fd_provision_t;

#define FD_PROVISION_OPTION_DEBUG_LOCK 0x00000001 
#define FD_PROVISION_OPTION_RESET 0x00000002 

#endif