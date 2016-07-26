#include "fdi_current_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instruments.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

typedef struct {
    uint64_t identifier;
    uint32_t enable;
    uint32_t channel;
    float multiplier;
} fdi_current_instrument_t;

#define fdi_current_instrument_count 1

static const uint64_t apiTypeConvert = 1;

fdi_current_instrument_t fdi_current_instruments[fdi_current_instrument_count];

fdi_current_instrument_t *fdi_current_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_current_instrument_count; ++i) {
        fdi_current_instrument_t *instrument = &fdi_current_instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_current_instrument_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_current_instrument_t *instrument = fdi_current_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float current = fdi_adc_convert(instrument->channel) * instrument->multiplier;

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, current);

    if (!fdi_api_send(instrument->identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_current_instrument_initialize(void) {
    fdi_current_instrument_t *instrument = &fdi_current_instruments[0];
    uint64_t identifier = fdi_instruments_register();
    instrument->identifier = identifier;
    instrument->enable = FDI_GPIO_ATE_USB_CS_EN;
    instrument->channel = 6; // USB current
    instrument->multiplier = fdi_current_sense_gain(0.1f, 1800.0f, 12000.0f, 376.0f, 1000.0f);
    fdi_api_register(identifier, apiTypeConvert, fdi_current_instrument_convert);
}