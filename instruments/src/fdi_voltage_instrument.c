#include "fdi_voltage_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instruments.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

typedef struct {
    uint64_t identifier;
    uint32_t channel;
    float multiplier;
} fdi_voltage_instrument_t;

#define fdi_voltage_instrument_count 3

static const uint64_t apiTypeConvert = 1;

fdi_voltage_instrument_t fdi_voltage_instruments[fdi_voltage_instrument_count];

fdi_voltage_instrument_t *fdi_voltage_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_voltage_instrument_count; ++i) {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_voltage_instrument_convert(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_voltage_instrument_t *instrument = fdi_voltage_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fdi_adc_convert(instrument->channel) * instrument->multiplier;

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, voltage);

    if (!fdi_api_send(instrument->identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_voltage_instrument_initialize(void) {
    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[0];
        uint64_t identifier = fdi_instruments_register();
        instrument->identifier = identifier;
        instrument->channel = 15; // battery voltage
        instrument->multiplier = 2.0f;
        fdi_api_register(identifier, apiTypeConvert, fdi_voltage_instrument_convert);
    }

    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[1];
        uint64_t identifier = fdi_instruments_register();
        instrument->identifier = identifier;
        instrument->channel = 3; // main rail voltage
        instrument->multiplier = 2.0f;
        fdi_api_register(identifier, apiTypeConvert, fdi_voltage_instrument_convert);
    }

    {
        fdi_voltage_instrument_t *instrument = &fdi_voltage_instruments[2];
        uint64_t identifier = fdi_instruments_register();
        instrument->identifier = identifier;
        instrument->channel = 7; // auxiliary rail voltage
        instrument->multiplier = 2.0f;
        fdi_api_register(identifier, apiTypeConvert, fdi_voltage_instrument_convert);
    }

}