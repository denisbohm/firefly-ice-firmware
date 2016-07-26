#include "fdi_instruments.h"

#include "fdi_api.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiIdentifierInstrumentManager = 0;

static const uint64_t apiTypeEcho = 1;
static const uint64_t apiTypeDiscoverInstruments = 2;

static uint64_t fdi_instruments_identifier;

void fdi_instruments_echo(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    if (!fdi_api_send(apiIdentifierInstrumentManager, apiTypeEcho, binary->buffer, binary->put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_instruments_discover_instruments(uint64_t identifier __attribute__((unused)), uint64_t type __attribute__((unused)), fd_binary_t *binary_in __attribute__((unused))) {
    uint8_t buffer[256];
    fd_binary_t binary;
    fd_binary_initialize(&binary, buffer, sizeof(buffer));
    fd_binary_put_varuint(&binary, 1); // 1 instrument
    
    // instrument
    fd_binary_put_string(&binary, "Indicator");
    fd_binary_put_varuint(&binary, 1);

    if (!fdi_api_send(apiIdentifierInstrumentManager, apiTypeDiscoverInstruments, binary.buffer, binary.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

uint64_t fdi_instruments_register(void) {
    return ++fdi_instruments_identifier;
}

void fdi_instruments_initialize(void) {
    fdi_instruments_identifier = 0;

    fdi_api_register(apiIdentifierInstrumentManager, apiTypeDiscoverInstruments, fdi_instruments_discover_instruments);
}