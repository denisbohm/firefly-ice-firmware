#include "fdi_current_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;
static const uint64_t apiTypeSetEnabled = 2;

#define fdi_current_instrument_count 1
fdi_current_instrument_t fdi_current_instruments[fdi_current_instrument_count];

uint32_t fdi_current_instrument_get_count(void) {
    return fdi_current_instrument_count;
}

fdi_current_instrument_t *fdi_current_instrument_get_at(uint32_t index) {
    return &fdi_current_instruments[index];
}

fdi_current_instrument_t *fdi_current_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_current_instrument_count; ++i) {
        fdi_current_instrument_t *instrument = &fdi_current_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_current_instrument_reset(fdi_current_instrument_t *instrument) {
    fdi_gpio_off(instrument->enable);
}

void fdi_current_instrument_api_reset(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_current_instrument_t *instrument = fdi_current_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_current_instrument_reset(instrument);
}

float fdi_current_instrument_convert(fdi_current_instrument_t *instrument) {
    return fdi_adc_convert(instrument->channel) * instrument->multiplier;
}

void fdi_current_instrument_api_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_current_instrument_t *instrument = fdi_current_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float current = fdi_current_instrument_convert(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, current);

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_current_instrument_set_enabled(fdi_current_instrument_t *instrument, bool enabled) {
    fdi_gpio_set(instrument->enable, enabled);
}

void fdi_current_instrument_api_set_enabled(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_current_instrument_t *instrument = fdi_current_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool enabled = fd_binary_get_uint8(binary) != 0;
    fdi_current_instrument_set_enabled(instrument, enabled);
}

void fdi_current_instrument_initialize(void) {
    fdi_current_instrument_t *instrument = &fdi_current_instruments[0];
    instrument->super.category = "Current";
    instrument->super.reset = fdi_current_instrument_api_reset;
    instrument->enable = FDI_GPIO_ATE_USB_CS_EN;
#ifdef FDI_INSTRUMENT_POWER
    instrument->channel = 10; // PA5 ADC12_IN10 : USB current
#endif
#ifdef FDI_INSTRUMENT_ALL_IN_ONE
    instrument->channel = 6; // USB current
#endif
    instrument->multiplier = fdi_current_sense_gain(0.1f, 1800.0f, 12000.0f, 376.0f, 1000.0f);
    fdi_instrument_register(&instrument->super);
    fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_current_instrument_api_reset);
    fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_current_instrument_api_convert);
    fdi_api_register(instrument->super.identifier, apiTypeSetEnabled, fdi_current_instrument_api_set_enabled);
}