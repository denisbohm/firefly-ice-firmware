#include "fd_hal_aes.h"
#include "fd_hal_external_flash.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_sha.h"
#include "fd_update.h"

#include <string.h>

void fd_update_read_crypto_key(uint8_t area, uint8_t *key) {
    memcpy(key, (uint8_t *)fd_hal_processor_get_crypto_range(area).address, FD_HAL_AES_KEY_SIZE);
}

void fd_update_read_metadata(uint8_t area, fd_update_metadata_t *metadata) {
    memcpy(metadata, (fd_update_metadata_t *)fd_hal_processor_get_firmware_update_metadata_range(area).address, sizeof(fd_update_metadata_t));
}

void fd_update_write_metadata(uint8_t area, fd_update_metadata_t *metadata) {
    fd_hal_processor_write_flash_data(fd_hal_processor_get_firmware_update_metadata_range(area).address, (uint8_t *)metadata, sizeof(fd_update_metadata_t));
}

void fd_update_get_external_hash(uint8_t area, uint32_t address, uint32_t length, uint8_t *hash) {
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    fd_hal_external_flash_wake();
    fd_sha1(fd_hal_external_flash_read, data_base_address + address, length, hash);
    fd_hal_external_flash_sleep();
}

void fd_update_get_sector_hash(uint8_t area, uint32_t sector, uint8_t *hash) {
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    uint32_t sector_size = fd_hal_external_flash_get_pages_per_sector() * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    fd_hal_external_flash_wake();
    fd_sha1(fd_hal_external_flash_read, data_base_address + sector * sector_size, sector_size, hash);
    fd_hal_external_flash_sleep();
}

void fd_update_erase_sector(uint8_t area, uint32_t sector) {
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    uint32_t sector_size = fd_hal_external_flash_get_pages_per_sector() * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    uint32_t address = data_base_address + sector * sector_size;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_enable_write();
    fd_hal_external_flash_erase_sector(address);
    fd_hal_external_flash_sleep();
}

void fd_update_write_page(uint8_t area, uint32_t page, uint8_t *data) {
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    uint32_t address = data_base_address + page * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_enable_write();
    fd_hal_external_flash_write_page(address, data, FD_HAL_EXTERNAL_FLASH_PAGE_SIZE);
    fd_hal_external_flash_sleep();
}

void fd_update_read_page(uint8_t area, uint32_t page, uint8_t *data) {
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    uint32_t address = data_base_address + page * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_read(address, data, FD_HAL_EXTERNAL_FLASH_PAGE_SIZE);
    fd_hal_external_flash_sleep();
}

static fd_hal_aes_decrypt_t decrypt;

static void decrypt_get(uint32_t address, uint8_t *data, uint32_t length) {
    fd_hal_external_flash_read(address, data, length);
    fd_hal_aes_decrypt_blocks(&decrypt, data, data, length);
}

uint8_t fd_update_commit(uint8_t area, fd_update_metadata_t *metadata) {
    uint8_t external_hash[FD_SHA_HASH_SIZE];
    fd_hal_external_flash_wake();
    uint32_t data_base_address = fd_hal_system_get_firmware_update_range(area).address;
    fd_sha1(fd_hal_external_flash_read, data_base_address, metadata->length, external_hash);
    fd_hal_external_flash_sleep();
    if (!fd_sha1_is_equal(metadata->hash, external_hash)) {
        return FD_UPDATE_COMMIT_FAIL_HASH_MISMATCH;
    }

    if (metadata->flags & FD_UPDATE_METADATA_FLAG_ENCRYPTED) {
        uint8_t crypt_hash[FD_SHA_HASH_SIZE];
        uint8_t key[FD_HAL_AES_KEY_SIZE];
        fd_update_read_crypto_key(area, key);
        fd_hal_aes_decrypt_start(&decrypt, key, metadata->crypt_iv);
        fd_hal_external_flash_wake();
        fd_sha1(decrypt_get, data_base_address, metadata->length, crypt_hash);
        fd_hal_external_flash_sleep();
        fd_hal_aes_decrypt_stop(&decrypt);
        if (!fd_sha1_is_equal(crypt_hash, metadata->crypt_hash)) {
            return FD_UPDATE_COMMIT_FAIL_CRYPT_HASH_MISMATCH;
        }
    }

#ifdef DEBUG
#warning debug is defined - firmware update commit is not enabled
    return FD_UPDATE_COMMIT_FAIL_UNSUPPORTED;
#endif

    if (memcmp((void *)fd_hal_processor_get_firmware_update_metadata_range(area).address, metadata, sizeof(fd_update_metadata_t)) != 0) {
        fd_update_write_metadata(area, metadata);
    }

    // on the next reset the boot loader will notice the new version and decrypt before executing

    return FD_UPDATE_COMMIT_SUCCESS;
}
