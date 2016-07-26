#include "fdi_battery_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instruments.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

typedef struct {
    uint64_t identifier;
    uint32_t enable;
    uint32_t channel_high;
    float multiplier_high;
    uint32_t channel_low;
    float multiplier_low;
} fdi_battery_instrument_t;

#define fdi_battery_instrument_count 1

static const uint64_t apiTypeConvert = 1;

fdi_battery_instrument_t fdi_battery_instruments[fdi_battery_instrument_count];

fdi_battery_instrument_t *fdi_battery_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_battery_instrument_count; ++i) {
        fdi_battery_instrument_t *instrument = &fdi_battery_instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_battery_instrument_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float current_high = fdi_adc_convert(instrument->channel_high) * instrument->multiplier_high;
    float current_low = fdi_adc_convert(instrument->channel_low) * instrument->multiplier_low;
    float current = current_high > 0.000250f ? current_high : current_low;

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, current);

    if (!fdi_api_send(instrument->identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_battery_instrument_set_voltage(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fd_binary_get_float32(binary);
    if (voltage < 2.75f) {
        voltage = 2.75f;
    }
    if (voltage > 4.2f) {
        voltage = 4.2f;
    }
    // 0x0222 = 2.75 V (battery cut-off)
    // ? = 4.2 V (max value)
    float multiplier = 1.0f; // ?
    uint16_t value = 0x0222 - (int)((voltage - 2.75f) * multiplier);
    fdi_mcp4726_write_volatile_dac_register(value);
}

void fdi_battery_instrument_initialize(void) {
    fdi_battery_instrument_t *instrument = &fdi_battery_instruments[0];
    uint64_t identifier = fdi_instruments_register();
    instrument->identifier = identifier;
    instrument->enable = FDI_GPIO_ATE_BS_EN;
    instrument->channel_high = 4; // battery current - high range
    instrument->multiplier_high = fdi_current_sense_gain(4.7f, 1800.0f, 12000.0f, 376.0f, 1000.0f);
    instrument->channel_low = 5; // battery current - low range
    instrument->multiplier_low = fdi_current_sense_gain(4.7f, 4.7f, 10000.0f, 0.0f, 1000.0f);
    fdi_api_register(identifier, apiTypeConvert, fdi_battery_instrument_convert);
    fdi_api_register(identifier, apiTypeConvert, fdi_battery_instrument_set_voltage);
}