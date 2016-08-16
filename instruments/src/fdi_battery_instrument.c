#include "fdi_battery_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;
static const uint64_t apiTypeSetVoltage = 2;
static const uint64_t apiTypeSetEnabled = 3;

#define fdi_battery_instrument_count 1
fdi_battery_instrument_t fdi_battery_instruments[fdi_battery_instrument_count];

uint32_t fdi_battery_instrument_get_count(void) {
    return fdi_battery_instrument_count;
}

fdi_battery_instrument_t *fdi_battery_instrument_get_at(uint32_t index) {
    return &fdi_battery_instruments[index];
}

fdi_battery_instrument_t *fdi_battery_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_battery_instrument_count; ++i) {
        fdi_battery_instrument_t *instrument = &fdi_battery_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_battery_instrument_reset(fdi_battery_instrument_t *instrument) {
    fdi_gpio_off(instrument->enable);
}

void fdi_battery_instrument_api_reset(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_battery_instrument_reset(instrument);
}

float fdi_battery_instrument_convert(fdi_battery_instrument_t *instrument) {
    float current_high = fdi_adc_convert(instrument->channel_high) * instrument->multiplier_high;
    float current_low = fdi_adc_convert(instrument->channel_low) * instrument->multiplier_low;
    float current = current_high > 0.000250f ? current_high : current_low;
    return current;
}

void fdi_battery_instrument_api_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float current = fdi_battery_instrument_convert(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, current);

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_battery_instrument_set_voltage(fdi_battery_instrument_t *instrument __attribute__((unused)), float voltage) {
    if (voltage < 2.75f) {
        voltage = 2.75f;
    }
    if (voltage > 4.2f) {
        voltage = 4.2f;
    }
    // 0x0222 = 2.75 V (battery cut-off)
    // 0x010d = 4.2 V (max value)
    float multiplier = ((float)(0x0222 - 0x010d)) / (4.2f - 2.75f);
    uint16_t value = 0x0222 - (int)((voltage - 2.75f) * multiplier);
    fdi_mcp4726_write_volatile_dac_register(value);
}

void fdi_battery_instrument_api_set_voltage(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fd_binary_get_float32(binary);

    fdi_battery_instrument_set_voltage(instrument, voltage);
}

void fdi_battery_instrument_set_enabled(fdi_battery_instrument_t *instrument __attribute__((unused)), bool enabled) {
    fdi_gpio_set(instrument->enable, enabled);
}

void fdi_battery_instrument_api_set_enabled(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool enabled = fd_binary_get_uint8(binary) != 0;
    fdi_battery_instrument_set_enabled(instrument, enabled);
}

void fdi_battery_instrument_initialize(void) {
    fdi_battery_instrument_t *instrument = &fdi_battery_instruments[0];
    instrument->super.category = "Battery";
    instrument->super.reset = fdi_battery_instrument_api_reset;
    instrument->enable = FDI_GPIO_ATE_BS_EN;
    instrument->channel_high = 4; // battery current - high range
    instrument->multiplier_high = fdi_current_sense_gain(4.7f, 1800.0f, 12000.0f, 376.0f, 1000.0f);
    instrument->channel_low = 5; // battery current - low range
    instrument->multiplier_low = fdi_current_sense_gain(4.7f, 4.7f, 10000.0f, 0.0f, 1000.0f);
    fdi_instrument_register(&instrument->super);
    fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_battery_instrument_api_reset);
    fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_battery_instrument_api_convert);
    fdi_api_register(instrument->super.identifier, apiTypeSetVoltage, fdi_battery_instrument_api_set_voltage);
    fdi_api_register(instrument->super.identifier, apiTypeSetEnabled, fdi_battery_instrument_api_set_enabled);
}