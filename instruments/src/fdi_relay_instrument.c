#include "fdi_relay_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instruments.h"
#include "fdi_relay.h"

#include "fd_binary.h"

typedef struct {
    uint64_t identifier;
    uint32_t control;
} fdi_relay_instrument_t;

#define fdi_relay_instrument_count 1

static const uint64_t apiTypeSet = 1;

fdi_relay_instrument_t fdi_relay_instruments[fdi_relay_instrument_count];

fdi_relay_instrument_t *fdi_relay_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_relay_instrument_count; ++i) {
        fdi_relay_instrument_t *instrument = &fdi_relay_instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_relay_instrument_set(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_relay_instrument_t *instrument = fdi_relay_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool on = fd_binary_get_uint8(binary) != 0;
    fdi_relay_set(instrument->control, on);
}

void fdi_relay_instrument_initialize(void) {
    fdi_relay_instrument_t *instrument = &fdi_relay_instruments[0];
    uint64_t identifier = fdi_instruments_register();
    instrument->identifier = identifier;
    instrument->control = FDI_RELAY_ATE_BUTTON_EN;
    fdi_api_register(identifier, apiTypeSet, fdi_relay_instrument_set);
}