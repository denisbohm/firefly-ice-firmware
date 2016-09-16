#include "fdi_instrument.h"

#include "fdi_api.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiIdentifierInstrumentManager = 0;

static const uint64_t apiTypeResetInstruments = 0;
static const uint64_t apiTypeDiscoverInstruments = 1;
static const uint64_t apiTypeEcho = 2;

#define FDI_INSTRUMENT_LIMIT 32

static uint32_t fdi_instrument_count;
static fdi_instrument_t *fdi_instruments[FDI_INSTRUMENT_LIMIT];

void fdi_instrument_echo(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    uint8_t *data = &binary->buffer[binary->get_index];
    uint32_t length = binary->size - binary->get_index;
    if (!fdi_api_send(apiIdentifierInstrumentManager, apiTypeEcho, data, length)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_instrument_discover_instruments(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary_in __attribute__((unused))) {
    uint8_t buffer[256];
    fd_binary_t binary;
    fd_binary_initialize(&binary, buffer, sizeof(buffer));
    fd_binary_put_varuint(&binary, fdi_instrument_count);
    
    for (uint32_t i = 0; i < fdi_instrument_count; ++i) {
        fdi_instrument_t *instrument = fdi_instruments[i];
        fd_binary_put_string(&binary, instrument->category);
        fd_binary_put_varuint(&binary, instrument->identifier);
    }

    if (!fdi_api_send(apiIdentifierInstrumentManager, apiTypeDiscoverInstruments, binary.buffer, binary.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_instrument_reset_instruments(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary_in __attribute__((unused))) {
    for (uint32_t i = 0; i < fdi_instrument_count; ++i) {
        fdi_instrument_t *instrument = fdi_instruments[i];
        if (instrument->reset) {
            instrument->reset(instrument->identifier, 0, 0);
        }
    }
}

void fdi_instrument_register(fdi_instrument_t *instrument) {
    if (fdi_instrument_count >= FDI_INSTRUMENT_LIMIT) {
        return;
    }
    fdi_instruments[fdi_instrument_count] = instrument;
    instrument->identifier = ++fdi_instrument_count;
}

void fdi_instrument_initialize(void) {
    fdi_instrument_count = 0;

    fdi_api_register(apiIdentifierInstrumentManager, apiTypeResetInstruments, fdi_instrument_reset_instruments);
    fdi_api_register(apiIdentifierInstrumentManager, apiTypeEcho, fdi_instrument_echo);
    fdi_api_register(apiIdentifierInstrumentManager, apiTypeDiscoverInstruments, fdi_instrument_discover_instruments);
}