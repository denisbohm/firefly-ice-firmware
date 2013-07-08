#include "fd_aes.h"
#include "fd_sha.h"
#include "fd_update.h"
#include "fd_w25q16dw.h"

#define FD_UPDATE_DATA_BASE_ADDRESS 0x00000000

#define SECTOR_SIZE (FD_W25Q16DW_PAGES_PER_SECTOR * FD_W25Q16DW_PAGE_SIZE)
#define BLOCKS_PER_SECTOR (SECTOR_SIZE / SHA1_BLOCK_LENGTH)

void fd_update_get_sector_hash(uint32_t sector, uint8_t *hash) {
    fd_sha1(fd_w25q16dw_read, FD_UPDATE_DATA_BASE_ADDRESS + sector * SECTOR_SIZE, SECTOR_SIZE, hash);
}

void fd_update_erase_sector(uint32_t sector) {
    uint32_t address = FD_UPDATE_DATA_BASE_ADDRESS + sector * SECTOR_SIZE;
    fd_w25q16dw_enable_write();
    fd_w25q16dw_erase_sector(address);
}

void fd_update_write_page(uint32_t page, uint8_t *data) {
    uint32_t address = FD_UPDATE_DATA_BASE_ADDRESS + page * FD_W25Q16DW_PAGE_SIZE;
    fd_w25q16dw_enable_write();
    fd_w25q16dw_write_page(address, data, FD_W25Q16DW_PAGE_SIZE);
}

static fd_aes_decrypt_t decrypt;

static void decrypt_get(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_read(address, data, length);
    fd_aes_decrypt_blocks(&decrypt, data, data, length);
}

#define USER_DATA_ADDRESS 0x0fe00000
#define KEY_ADDRESS (USER_DATA_ADDRESS + 4)

uint8_t fd_update_commit(fd_update_metadata_t *metadata) {
    uint8_t hash[20];
    fd_sha1(fd_w25q16dw_read, FD_UPDATE_DATA_BASE_ADDRESS, metadata->length, hash);
    if (!fd_sha1_is_equal(hash, metadata->hash)) {
        return FD_UPDATE_COMMIT_FAIL_HASH_MISMATCH;
    }

    if (metadata->flags & FD_UPDATE_METADATA_FLAG_ENCRYPTED) {
        uint8_t crypt_hash[20];
        uint8_t *key = (uint8_t *)KEY_ADDRESS;
        fd_aes_decrypt_start(&decrypt, key, metadata->crypt_iv);
        fd_sha1(decrypt_get, FD_UPDATE_DATA_BASE_ADDRESS, metadata->length, crypt_hash);
        fd_aes_decrypt_stop(&decrypt);
        if (!fd_sha1_is_equal(hash, metadata->crypt_hash)) {
            return FD_UPDATE_COMMIT_FAIL_CRYPT_HASH_MISMATCH;
        }
    }

    // jump into boot loader to copy (decrypted) firmware to microcontroller flash & do a system reset

    return 0;
}
