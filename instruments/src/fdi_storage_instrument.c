#include "fdi_storage_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#ifdef FD_INSTRUMENT_ALL_IN_ONE
#include "fdi_s25fl116k.h"
#endif

#include "fd_binary.h"
#include "fd_log.h"
#include "fd_sdcard.h"
#include "fd_sha.h"

#include "ff.h"

#include <string.h>

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeErase = 1;
static const uint64_t apiTypeWrite = 2;
static const uint64_t apiTypeRead = 3;
static const uint64_t apiTypeHash = 4;
static const uint64_t apiTypeFileMkfs = 5;
static const uint64_t apiTypeFileList = 6;
static const uint64_t apiTypeFileOpen = 7;
static const uint64_t apiTypeFileUnlink = 8;
static const uint64_t apiTypeFileAddress = 9;
static const uint64_t apiTypeFileExpand = 10;
static const uint64_t apiTypeFileWrite = 11;
static const uint64_t apiTypeFileRead = 12;

#define fdi_storage_instrument_count 1
fdi_storage_instrument_t fdi_storage_instruments[fdi_storage_instrument_count];

uint32_t fdi_storage_instrument_get_count(void) {
    return fdi_storage_instrument_count;
}

fdi_storage_instrument_t *fdi_storage_instrument_get_at(uint32_t index) {
    return &fdi_storage_instruments[index];
}

fdi_storage_instrument_t *fdi_storage_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_storage_instrument_count; ++i) {
        fdi_storage_instrument_t *instrument = &fdi_storage_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_storage_instrument_reset(fdi_storage_instrument_t *instrument __attribute__((unused))) {
    // nothing to do...
}

void fdi_storage_instrument_api_reset(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_storage_instrument_reset(instrument);
}

void fdi_storage_instrument_api_erase(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
 
#ifdef FD_INSTRUMENT_ALL_IN_ONE
    uint32_t end = address + length;
    while (address < end) {
        fdi_s25fl116k_enable_write();
        fdi_s25fl116k_erase_sector(address);
        address += fdi_s25fl116k_sector_size;
        fdi_s25fl116k_wait_while_busy();
    }
#else
    fd_sdcard_erase(address, length);
#endif
}

void fdi_storage_instrument_api_write(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);

#ifdef FD_INSTRUMENT_ALL_IN_ONE
    uint32_t offset = 0;
    while (offset < length) {
        uint32_t sublength = length - offset;
        uint32_t lower_address = address + offset;
        uint32_t lower_page = lower_address >> 8;
        uint32_t upper_page = (lower_address + sublength - 1) >> 8;
        if (upper_page != lower_page) {
            sublength = ((lower_page + 1) << 8) - lower_address;
        }
        fdi_s25fl116k_enable_write();
        fdi_s25fl116k_write_page(lower_address, &binary->buffer[binary->get_index], sublength);
        fdi_s25fl116k_wait_while_busy();
        binary->get_index += sublength;
        offset += sublength;
    }
#else
    fd_sdcard_write(address, &binary->buffer[binary->get_index], length);
#endif
}

void fdi_storage_instrument_read(
    fdi_storage_instrument_t *instrument,
    uint32_t address, uint32_t length, uint32_t sublength, uint32_t substride,
    uint8_t *buffer, uint32_t size
) {
    fd_log_assert(length <= size);
    if (length > size) {
        length = size;
    }
    if (sublength == 0) {
        sublength = length;
    }

    uint32_t total = 0;
    while (total < length) {
#ifdef FD_INSTRUMENT_ALL_IN_ONE
        fdi_s25fl116k_read(address, &buffer[total], sublength);
#else
        fd_sdcard_read(address, &buffer[total], sublength);
#endif
        total += sublength;
        address += substride;
    }
}

void fdi_storage_instrument_api_read(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t sublength = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t substride = (uint32_t)fd_binary_get_varuint(binary);
    uint8_t buffer[4096];
    fdi_storage_instrument_read(instrument, address, length, sublength, substride, buffer, sizeof(buffer));

    if (!fdi_api_send(instrument->super.identifier, apiTypeRead, buffer, length)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_storage_instrument_api_hash(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);

    uint8_t hash[FD_SHA_HASH_SIZE];
#ifdef FD_INSTRUMENT_ALL_IN_ONE
    fd_sha1(fdi_s25fl116k_read, address, length, hash);
#else
    fd_sha1(fd_sdcard_read, address, length, hash);
#endif

    if (!fdi_api_send(instrument->super.identifier, apiTypeHash, hash, FD_SHA_HASH_SIZE)) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_mkfs(fdi_storage_instrument_t *instrument) {
    f_mount(0, "", 1);
    uint8_t mkfs_buffer[4096];
    FRESULT result = f_mkfs("", 0, mkfs_buffer, sizeof(mkfs_buffer));
    if (result == FR_OK) {
        result = f_mount(&instrument->fs, "", 1);
    }
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_mkfs(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool result = fdi_storage_instrument_file_mkfs(instrument);
    uint8_t buffer[] = { result ? 1 : 0 };
    if (!fdi_api_send(instrument->super.identifier, apiTypeFileMkfs, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_storage_instrument_api_file_list(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint8_t buffer[4096];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    uint8_t count = 0;
    fd_binary_put_uint8(&response, count);
    DIR dir;
    FRESULT result = f_opendir(&dir, "/");
    if (result == FR_OK) {
        while (true) {
            FILINFO info;
            result = f_readdir(&dir, &info);
            if ((result != FR_OK) || (info.fname[0] == 0)) {
                break;
            }
            if ((info.fattrib & AM_DIR) == 0) {
                fd_binary_put_string(&response, info.fname);
                fd_binary_put_uint32(&response, info.fsize);
                fd_binary_put_uint32(&response, info.fdate);
                fd_binary_put_uint32(&response, info.ftime);
                ++count;
            }
        }
        f_closedir(&dir);
    }
    buffer[0] = count;
    if (!fdi_api_send(instrument->super.identifier, apiTypeFileList, buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_open(fdi_storage_instrument_t *instrument, const char *name, uint8_t mode) {
    FIL file;
    FRESULT result = f_open(&file, name, mode);
    if (result == FR_OK) {
        f_close(&file);
    }
    return result == FR_OK;
}

static void fdi_storage_instrument_get_name(fd_binary_t *binary, char *name, size_t size) {
    fd_binary_string_t string = fd_binary_get_string(binary);
    if (string.length > (size - 1)) {
        string.length = size - 1;
    }
    memcpy(name, string.data, string.length);
    name[string.length] = '\0';
}

void fdi_storage_instrument_api_file_open(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    uint8_t mode = (uint8_t)fd_binary_get_varuint(binary);
    bool result = fdi_storage_instrument_file_open(instrument, name, mode);
    uint8_t buffer[] = { result ? 1 : 0 };

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileOpen, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_unlink(fdi_storage_instrument_t *instrument, const char *name) {
    FRESULT result = f_unlink(name);
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_unlink(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    bool result = fdi_storage_instrument_file_unlink(instrument, name);
    uint8_t buffer[] = { result ? 1 : 0 };

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileUnlink, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_address(fdi_storage_instrument_t *instrument, const char *name, uint32_t *address) {
    FIL file;
    FRESULT result = f_open(&file, name, FA_READ | FA_OPEN_EXISTING);
    if (result == FR_OK) {
        *address = file.obj.fs->database + file.obj.fs->csize * (file.obj.sclust - 2);
    }
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_address(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    uint32_t address = 0;
    bool result = fdi_storage_instrument_file_address(instrument, name, &address);
    uint8_t buffer[] = { result ? 1 : 0, address, address >> 8, address >> 16, address >> 24 };

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileAddress, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_expand(fdi_storage_instrument_t *instrument, const char *name, uint32_t size) {
    FIL file;
    FRESULT result = f_open(&file, name, FA_WRITE | FA_OPEN_EXISTING);
    if (result == FR_OK) {
        result = f_expand(&file, size, 1);
        f_close(&file);
    }
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_expand(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    uint32_t size = fd_binary_get_uint32(binary);
    bool result = fdi_storage_instrument_file_expand(instrument, name, size);
    uint8_t buffer[] = { result ? 1 : 0 };

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileExpand, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_write(fdi_storage_instrument_t *instrument, const char *name, uint32_t offset, const uint8_t *data, uint32_t length) {
    FIL file;
    FRESULT result = f_open(&file, name, FA_WRITE | FA_OPEN_EXISTING);
    if (result == FR_OK) {
        result = f_lseek(&file, offset);
        if (result == FR_OK) {
            uint32_t actual = 0;
            result = f_write(&file, data, length, &actual);
            if (result == FR_OK) {
                if (actual != length) {
                    result = FR_INVALID_PARAMETER;
                }
            }
        }
        f_close(&file);
    }
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_write(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    uint32_t offset = fd_binary_get_uint32(binary);
    uint32_t length = fd_binary_get_uint32(binary);
    uint8_t *data = &binary->buffer[binary->get_index];
    bool result = fdi_storage_instrument_file_write(instrument, name, offset, data, length);
    uint8_t buffer[] = { result ? 1 : 0 };

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileWrite, buffer, sizeof(buffer))) {
        fd_log_assert_fail("can't send");
    }
}

bool fdi_storage_instrument_file_read(fdi_storage_instrument_t *instrument, const char *name, uint32_t offset, uint8_t *data, uint32_t length) {
    FIL file;
    FRESULT result = f_open(&file, name, FA_READ | FA_OPEN_EXISTING);
    if (result == FR_OK) {
        result = f_lseek(&file, offset);
        if (result == FR_OK) {
            uint32_t actual = 0;
            result = f_read(&file, data, length, &actual);
            if (result == FR_OK) {
                if (actual != length) {
                    result = FR_INVALID_PARAMETER;
                }
            }
        }
        f_close(&file);
    }
    return result == FR_OK;
}

void fdi_storage_instrument_api_file_read(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    char name[12 + 1];
    fdi_storage_instrument_get_name(binary, name, sizeof(name));
    uint32_t offset = fd_binary_get_uint32(binary);
    uint32_t length = fd_binary_get_uint32(binary);
    uint8_t data[1 + 4 + 4096];
    data[1] = length;
    data[2] = length >> 8;
    data[3] = length >> 16;
    data[4] = length >> 24;
    bool result = fdi_storage_instrument_file_read(instrument, name, offset, &data[1 + 4], length);
    data[0] = result ? 1 : 0;

    if (!fdi_api_send(instrument->super.identifier, apiTypeFileRead, data, 1 + 4 + length)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_storage_instrument_initialize(void) {
    fdi_storage_instrument_t *instrument = &fdi_storage_instruments[0];
    instrument->super.category = "Storage";
    instrument->super.reset = fdi_storage_instrument_api_reset;
    fdi_instrument_register(&instrument->super);
    fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_storage_instrument_api_reset);
    fdi_api_register(instrument->super.identifier, apiTypeErase, fdi_storage_instrument_api_erase);
    fdi_api_register(instrument->super.identifier, apiTypeWrite, fdi_storage_instrument_api_write);
    fdi_api_register(instrument->super.identifier, apiTypeRead, fdi_storage_instrument_api_read);
    fdi_api_register(instrument->super.identifier, apiTypeHash, fdi_storage_instrument_api_hash);
    fdi_api_register(instrument->super.identifier, apiTypeFileMkfs, fdi_storage_instrument_api_file_mkfs);
    fdi_api_register(instrument->super.identifier, apiTypeFileList, fdi_storage_instrument_api_file_list);
    fdi_api_register(instrument->super.identifier, apiTypeFileOpen, fdi_storage_instrument_api_file_open);
    fdi_api_register(instrument->super.identifier, apiTypeFileUnlink, fdi_storage_instrument_api_file_unlink);
    fdi_api_register(instrument->super.identifier, apiTypeFileAddress, fdi_storage_instrument_api_file_address);
    fdi_api_register(instrument->super.identifier, apiTypeFileExpand, fdi_storage_instrument_api_file_expand);
    fdi_api_register(instrument->super.identifier, apiTypeFileWrite, fdi_storage_instrument_api_file_write);
    fdi_api_register(instrument->super.identifier, apiTypeFileRead, fdi_storage_instrument_api_file_read);
    FRESULT result = f_mount(&instrument->fs, "", 1);
    fd_log_assert(result == FR_OK);
}