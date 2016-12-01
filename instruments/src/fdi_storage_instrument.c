#include "fdi_storage_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_s25fl116k.h"

#include "fd_binary.h"
#include "fd_log.h"
#include "fd_sha.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeErase = 1;
static const uint64_t apiTypeWrite = 2;
static const uint64_t apiTypeRead = 3;
static const uint64_t apiTypeHash = 4;

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
 
    uint32_t end = address + length;
    while (address < end) {
        fdi_s25fl116k_enable_write();
        fdi_s25fl116k_erase_sector(address);
        address += fdi_s25fl116k_sector_size;
    }
    fdi_s25fl116k_wait_while_busy();
}

void fdi_storage_instrument_api_write(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_storage_instrument_t *instrument = fdi_storage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t address = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t length = (uint32_t)fd_binary_get_varuint(binary);

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
        binary->get_index += sublength;
        offset += sublength;
    }
    fdi_s25fl116k_wait_while_busy();
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
    fd_log_assert(length <= 4096);
    if (length > 4096) {
        length = 4096;
    }
    if (sublength == 0) {
        sublength = length;
    }

    uint8_t buffer[4096];
    uint32_t total = 0;
    while (total < length) {
        fdi_s25fl116k_read(address, &buffer[total], sublength);
        total += sublength;
        address += substride;
    }

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
    fd_sha1(fdi_s25fl116k_read, address, length, hash);

    if (!fdi_api_send(instrument->super.identifier, apiTypeHash, hash, FD_SHA_HASH_SIZE)) {
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
}