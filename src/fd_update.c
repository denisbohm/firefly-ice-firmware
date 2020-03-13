#include "fd_hal_aes.h"
#include "fd_hal_external_flash.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_sha.h"
#include "fd_update.h"

#include "sha.h"

#include <string.h>

void fd_update_sha_source(FD_SHA_POINTER_UINT_TYPE address, uint8_t *data, uint32_t length) {
    fd_hal_external_flash_read((uint32_t)address, data, length);
}

bool fd_update_check_range(fd_range_t range, uint32_t address, uint32_t length) {
    return (address + length) <= range.length;
}

void fd_update_get_external_hash(uint8_t area, uint32_t address, uint32_t length, uint8_t *hash) {
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    if (!fd_update_check_range(range, address, length)) {
        memcpy(hash, 0, FD_SHA_HASH_SIZE);
        return;
    }
    uint32_t data_base_address = range.address;
    fd_hal_external_flash_wake();
    fd_sha1(fd_update_sha_source, data_base_address + address, length, hash);
    fd_hal_external_flash_sleep();
}

void fd_update_get_sector_hash(uint8_t area, uint32_t sector, uint8_t *hash) {
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    uint32_t sector_size = fd_hal_external_flash_get_pages_per_sector() * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    if (((sector + 1) * sector_size) > range.length) {
        memcpy(hash, 0, FD_SHA_HASH_SIZE);
        return;
    }
    uint32_t data_base_address = range.address;
    fd_hal_external_flash_wake();
    fd_sha1(fd_update_sha_source, data_base_address + sector * sector_size, sector_size, hash);
    fd_hal_external_flash_sleep();
}

void fd_update_erase_sector(uint8_t area, uint32_t sector) {
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    uint32_t sector_size = fd_hal_external_flash_get_pages_per_sector() * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    if (((sector + 1) * sector_size) > range.length) {
        return;
    }
    uint32_t data_base_address = range.address;
    uint32_t address = data_base_address + sector * sector_size;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_enable_write();
    fd_hal_external_flash_erase_sector(address);
    fd_hal_external_flash_sleep();
}

void fd_update_write_page(uint8_t area, uint32_t page, uint8_t *data) {
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    if (((page + 1) * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE) > range.length) {
        return;
    }
    uint32_t data_base_address = range.address;
    uint32_t address = data_base_address + page * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_enable_write();
    fd_hal_external_flash_write_page(address, data, FD_HAL_EXTERNAL_FLASH_PAGE_SIZE);
    fd_hal_external_flash_sleep();
}

void fd_update_read_page(uint8_t area, uint32_t page, uint8_t *data) {
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    if (((page + 1) * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE) > range.length) {
        memcpy(data, 0, FD_HAL_EXTERNAL_FLASH_PAGE_SIZE);
        return;
    }
    uint32_t data_base_address = range.address;
    uint32_t address = data_base_address + page * FD_HAL_EXTERNAL_FLASH_PAGE_SIZE;
    fd_hal_external_flash_wake();
    fd_hal_external_flash_read(address, data, FD_HAL_EXTERNAL_FLASH_PAGE_SIZE);
    fd_hal_external_flash_sleep();
}

static fd_hal_aes_decrypt_t decrypt;

static void decrypt_get(FD_SHA_POINTER_UINT_TYPE address, uint8_t *data, uint32_t length) {
    uint8_t in[SHA1_BLOCK_LENGTH];
    fd_hal_external_flash_read((uint32_t)address, in, length);
    fd_hal_aes_decrypt_blocks(&decrypt, in, data, length);
}

uint8_t fd_update_commit(uint8_t area, fd_version_metadata_t *metadata) {
    uint8_t external_hash[FD_SHA_HASH_SIZE];
    fd_hal_external_flash_wake();
    fd_range_t range;
    fd_hal_system_get_update_external_flash_range(area, &range);
    if (metadata->binary.length > range.length) {
        return FD_UPDATE_COMMIT_FAIL_INVALID_METADATA;
    }
    uint32_t data_base_address = range.address;
    fd_sha1(fd_update_sha_source, data_base_address, metadata->binary.length, external_hash);
    fd_hal_external_flash_sleep();
    if (!fd_sha1_is_equal(metadata->binary.crypt_hash, external_hash)) {
        return FD_UPDATE_COMMIT_FAIL_CRYPT_HASH_MISMATCH;
    }

    if (metadata->binary.flags & FD_VERSION_METADATA_FLAG_ENCRYPTED) {
        uint8_t hash[FD_SHA_HASH_SIZE];
        uint8_t key[FD_HAL_AES_KEY_SIZE];
        fd_hal_system_get_crypto_key(area, key);
        fd_hal_aes_decrypt_start(&decrypt, key, metadata->binary.crypt_iv);
        fd_hal_external_flash_wake();
        fd_sha1(decrypt_get, data_base_address, metadata->binary.length, hash);
        fd_hal_external_flash_sleep();
        fd_hal_aes_decrypt_stop(&decrypt);
        if (!fd_sha1_is_equal(hash, metadata->binary.hash)) {
            return FD_UPDATE_COMMIT_FAIL_HASH_MISMATCH;
        }
    }

#ifndef __APPLE__
#ifdef FD_UPDATE_DEBUG
#warning FD_UPDATE_DEBUG is defined - firmware update commit is not enabled for primary
    if (area <= FD_HAL_SYSTEM_AREA_OPERATING_SYSTEM) {
        return FD_UPDATE_COMMIT_FAIL_UNSUPPORTED;
    }
#endif
#endif

    fd_version_metadata_t update_metadata;
    fd_hal_system_get_update_metadata(area, &update_metadata);
    if (memcmp(&update_metadata, metadata, sizeof(fd_version_metadata_t)) != 0) {
        fd_hal_system_set_update_metadata(area, metadata);
    }

    // on the next reset the boot loader will notice the new version and decrypt before executing

    return FD_UPDATE_COMMIT_SUCCESS;
}
