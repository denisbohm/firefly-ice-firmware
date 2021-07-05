#include "fdi_voltage_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;

#define fdi_voltage_instrument_count 3
fdi_voltage_instrument_t fdi_voltage_instruments[fdi_voltage_instrument_count];

uint32_t fdi_voltage_instrument_get_count(void) {
    return fdi_voltage_instrument_count;
}

fdi_voltage_instrument_t *fdi_voltage_instrument_get_at(uint32_t index) {
    return &fdi_voltage_instruments[index];
}

fdi_voltage_instrument_t *fdi_voltage_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_voltage_instrument_count; ++i) {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_voltage_instrument_reset(fdi_voltage_instrument_t *instrument) {
    // nothing to do...
}

void fdi_voltage_instrument_api_reset(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_voltage_instrument_t *instrument = fdi_voltage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_voltage_instrument_reset(instrument);
}

float fdi_relay_instrument_convert(fdi_voltage_instrument_t *instrument) {
    float voltage = fdi_adc_convert(instrument->channel) * instrument->multiplier;
    return voltage;
}

void fdi_voltage_instrument_api_convert(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_voltage_instrument_t *instrument = fdi_voltage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fdi_relay_instrument_convert(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, voltage);

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_voltage_instrument_initialize(void) {
    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[0];
        instrument->super.category = "Voltage";
        instrument->super.reset = fdi_voltage_instrument_api_reset;
        instrument->channel = 15; // battery voltage
        instrument->multiplier = 2.0f;
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_voltage_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_voltage_instrument_api_convert);
    }

    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[1];
        instrument->super.category = "Voltage";
        instrument->super.reset = fdi_voltage_instrument_api_reset;
        instrument->channel = 3; // main rail voltage
        instrument->multiplier = 2.0f;
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_voltage_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_voltage_instrument_api_convert);
    }

    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[2];
        instrument->super.category = "Voltage";
        instrument->super.reset = fdi_voltage_instrument_api_reset;
        instrument->channel = 7; // auxiliary rail voltage
        instrument->multiplier = 2.0f;
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_voltage_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_voltage_instrument_api_convert);
    }
}