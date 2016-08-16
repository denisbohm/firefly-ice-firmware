#include "fdi_relay_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_relay.h"

#include "fd_binary.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeSet = 1;

#define fdi_relay_instrument_count 5
fdi_relay_instrument_t fdi_relay_instruments[fdi_relay_instrument_count];

uint32_t fdi_relay_instrument_get_count(void) {
    return fdi_relay_instrument_count;
}

fdi_relay_instrument_t *fdi_relay_instrument_get_at(uint32_t index) {
    return &fdi_relay_instruments[index];
}

fdi_relay_instrument_t *fdi_relay_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_relay_instrument_count; ++i) {
        fdi_relay_instrument_t *instrument = &fdi_relay_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_relay_instrument_reset(fdi_relay_instrument_t *instrument) {
    fdi_relay_off(instrument->control);
}

void fdi_relay_instrument_api_reset(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_relay_instrument_t *instrument = fdi_relay_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_relay_instrument_reset(instrument);
}

void fdi_relay_instrument_set(fdi_relay_instrument_t *instrument, bool on) {
    fdi_relay_set(instrument->control, on);
}

void fdi_relay_instrument_api_set(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_relay_instrument_t *instrument = fdi_relay_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool on = fd_binary_get_uint8(binary) != 0;
    fdi_relay_instrument_set(instrument, on);
}

void fdi_relay_instrument_initialize(void) {
    uint32_t controls[] = {
        FDI_RELAY_ATE_BUTTON_EN,
        FDI_RELAY_ATE_USB_5V_EN,
        FDI_RELAY_ATE_USB_D_EN,
        FDI_RELAY_ATE_MCU_VCC_SENSE,
        FDI_RELAY_ATE_BATTERY_SENSE,
    };

    for (int i = 0; i < fdi_relay_instrument_count; ++i) {
        fdi_relay_instrument_t *instrument = &fdi_relay_instruments[i];
        instrument->super.category = "Relay";
        instrument->super.reset = fdi_relay_instrument_api_reset;
        instrument->control = controls[i];
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_relay_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeSet, fdi_relay_instrument_api_set);
    }
}