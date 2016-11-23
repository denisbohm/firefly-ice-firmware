#include "fdi_battery_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_interrupt.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;
static const uint64_t apiTypeSetVoltage = 2;
static const uint64_t apiTypeSetEnabled = 3;
static const uint64_t apiTypeConvertContinuous = 4;
static const uint64_t apiTypeConvertContinuousSample = 5;

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
//    fdi_adc_power_down(); // turn off any continuous conversions
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

#define fdi_battery_instrument_sample_size 2048
#define fdi_battery_instrument_sample_mask (fdi_battery_instrument_sample_size - 1)
volatile float fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_size];
volatile uint32_t fdi_battery_instrument_sample_head_index;
volatile uint32_t fdi_battery_instrument_sample_count;
volatile bool fdi_battery_instrument_overflow;
float fdi_battery_instrument_multiplier_high;
float fdi_battery_instrument_multiplier_low;
volatile double fdi_battery_instrument_total_current;
volatile uint32_t fdi_battery_instrument_total_count;
double fdi_battery_instrument_interval_current;
uint32_t fdi_battery_instrument_interval_count;
uint32_t fdi_battery_instrument_interval_size;

void fdi_battery_instrument_sample_inititialize(fdi_battery_instrument_t *instrument) {
    fdi_battery_instrument_sample_head_index = 0;
    fdi_battery_instrument_sample_count = 0;
    fdi_battery_instrument_overflow = false;
    fdi_battery_instrument_multiplier_high = (3.3f / 4096.0f) * instrument->multiplier_high;
    fdi_battery_instrument_multiplier_low = (3.3f / 4096.0f) * instrument->multiplier_low;
    fdi_battery_instrument_total_current = 0.0;
    fdi_battery_instrument_total_count = 0;
    fdi_battery_instrument_interval_current = 0.0;
    fdi_battery_instrument_interval_count = 0;
    fdi_battery_instrument_interval_size = 1000;
}

bool fdi_battery_instrument_sample_is_empty(void) {
    return fdi_battery_instrument_sample_count == 0;
}

bool fdi_battery_instrument_sample_is_full(void) {
    return fdi_battery_instrument_sample_count == fdi_battery_instrument_sample_size;
}

void fdi_battery_instrument_queue(float value) {
    fd_log_assert(!fdi_battery_instrument_sample_is_full());

    fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_head_index] = value;
    fdi_battery_instrument_sample_head_index = ((fdi_battery_instrument_sample_head_index + 1) & fdi_battery_instrument_sample_mask);
    ++fdi_battery_instrument_sample_count;
}

float fdi_battery_instrument_dequeue(void) {
    fd_log_assert(!fdi_battery_instrument_sample_is_empty());

    uint32_t index;
    if (fdi_battery_instrument_sample_count <= fdi_battery_instrument_sample_head_index) {
        index = fdi_battery_instrument_sample_head_index - fdi_battery_instrument_sample_count;
    } else {
        index = fdi_battery_instrument_sample_size + fdi_battery_instrument_sample_head_index - fdi_battery_instrument_sample_count;
    }
    float value = fdi_battery_instrument_sample_buffer[index];
    --fdi_battery_instrument_sample_count;

    return value;
}

void fdi_battery_instrument_convert_continuous_callback(volatile uint16_t *results) {
    if (!fdi_battery_instrument_sample_is_full()) {
        uint16_t high_result = *results++;
        uint16_t low_result = *results;
        float current_high = high_result * fdi_battery_instrument_multiplier_high;
        float current_low = low_result * fdi_battery_instrument_multiplier_low;
        float current = current_high > 0.000250f ? current_high : current_low;

        fdi_battery_instrument_total_current += (double)current;
        ++fdi_battery_instrument_total_count;

        fdi_battery_instrument_interval_current += (double)current;
        ++fdi_battery_instrument_interval_count;
        if (fdi_battery_instrument_interval_count >= fdi_battery_instrument_interval_size) {
            fdi_battery_instrument_interval_current = 0.0;
            fdi_battery_instrument_interval_count = 0;
            fdi_battery_instrument_queue(current);
        }
    } else {
        fdi_battery_instrument_overflow = true;
    }
}

void fdi_battery_instrument_send_conversions(void) {
    uint32_t count = fdi_battery_instrument_sample_count;
    if (count <= 0) {
        return;
    }

    uint8_t buffer[300];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint32(&response, (1 /* enabled */) | (fdi_battery_instrument_overflow >> 1));
    if (count > 64) {
        count = 64;
    }
    fd_binary_put_varuint(&response, count);
    fdi_battery_instrument_t *instrument = &fdi_battery_instruments[0];
    while (count-- != 0) {
        uint32_t primask = fdi_interrupt_disable();
        float current = fdi_battery_instrument_dequeue();
        fdi_interrupt_restore(primask);
        fd_binary_put_float32(&response, current);
    }

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvertContinuousSample, response.buffer, response.put_index)) {
        fdi_battery_instrument_overflow = true;
        fd_log_assert_fail("can't send");
    }
}

#define FDI_BATTERY_INSTRUMENT_CONVERT_CONTINUOUS_FLAGS_START 0b01
#define FDI_BATTERY_INSTRUMENT_CONVERT_CONTINUOUS_FLAGS_STOP  0b10

void fdi_battery_instrument_api_convert_continuous(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t flags = fd_binary_get_uint32(binary);

    // !!! note that the code below doesn't share the ADC with others...  API user must beware... -denis
    if (flags & FDI_BATTERY_INSTRUMENT_CONVERT_CONTINUOUS_FLAGS_START) {
        fdi_battery_instrument_sample_inititialize(&fdi_battery_instruments[0]);

        fdi_adc_power_up();
        uint8_t channels[2] = { instrument->channel_high, instrument->channel_low };
        fdi_adc_convert_continuous(channels, sizeof(channels), fdi_battery_instrument_convert_continuous_callback);
    }
    if (flags & FDI_BATTERY_INSTRUMENT_CONVERT_CONTINUOUS_FLAGS_STOP) {
        fdi_adc_power_down();
    }

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint32(&response, flags);
    fd_binary_put_uint32(&response, fdi_battery_instrument_total_count);
    fd_binary_put_double64(&response, fdi_battery_instrument_total_current);

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvertContinuous, response.buffer, response.put_index)) {
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
    fdi_api_register(instrument->super.identifier, apiTypeConvertContinuous, fdi_battery_instrument_api_convert_continuous);
}