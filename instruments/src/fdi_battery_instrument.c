#include "fdi_battery_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_current.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_interrupt.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"
#include "fdi_s25fl116k.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <float.h>

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;
static const uint64_t apiTypeSetVoltage = 2;
static const uint64_t apiTypeSetEnabled = 3;
static const uint64_t apiTypeConvertContinuous = 4;
static const uint64_t apiTypeConvertContinuousComplete = 5;

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

uint32_t fdi_battery_instrument_sample_decimation;
uint32_t fdi_battery_instrument_sample_total;
uint32_t fdi_battery_instrument_sample_address;

double fdi_battery_instrument_multiplier_high;
double fdi_battery_instrument_multiplier_low;

typedef struct {
    uint32_t count;
    double minimum;
    double maximum;
    double mean;
    double S;
} fdi_battery_instrument_sample_t;

void fdi_battery_instrument_sample_clear(fdi_battery_instrument_sample_t *sample) {
    sample->count = 0;
    sample->minimum = DBL_MAX;
    sample->maximum = 0.0;
    sample->mean = 0.0;
    sample->S = 0.0;
}

fdi_battery_instrument_sample_t fdi_battery_instrument_sample;

uint32_t fdi_battery_instrument_total_count;

#define fdi_battery_instrument_sample_size 32
#define fdi_battery_instrument_sample_mask (fdi_battery_instrument_sample_size - 1)
volatile fdi_battery_instrument_sample_t fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_size];
volatile uint32_t fdi_battery_instrument_sample_head_index;
volatile uint32_t fdi_battery_instrument_sample_count;
volatile bool fdi_battery_instrument_sample_overflow;

bool fdi_battery_instrument_sample_is_empty(void) {
    return fdi_battery_instrument_sample_count == 0;
}

bool fdi_battery_instrument_sample_is_full(void) {
    return fdi_battery_instrument_sample_count == fdi_battery_instrument_sample_size;
}

void fdi_battery_instrument_sample_queue(fdi_battery_instrument_sample_t value) {
    fd_log_assert(!fdi_battery_instrument_sample_is_full());

    fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_head_index] = value;
    fdi_battery_instrument_sample_head_index = ((fdi_battery_instrument_sample_head_index + 1) & fdi_battery_instrument_sample_mask);
    ++fdi_battery_instrument_sample_count;
}

fdi_battery_instrument_sample_t fdi_battery_instrument_sample_dequeue(void) {
    fd_log_assert(!fdi_battery_instrument_sample_is_empty());

    uint32_t index;
    if (fdi_battery_instrument_sample_count <= fdi_battery_instrument_sample_head_index) {
        index = fdi_battery_instrument_sample_head_index - fdi_battery_instrument_sample_count;
    } else {
        index = fdi_battery_instrument_sample_size + fdi_battery_instrument_sample_head_index - fdi_battery_instrument_sample_count;
    }
    fdi_battery_instrument_sample_t value = fdi_battery_instrument_sample_buffer[index];
    --fdi_battery_instrument_sample_count;
    return value;
}

void fdi_battery_instrument_convert_continuous_callback(volatile uint16_t *results) {
#if 0
    fdi_battery_instrument_total_count += 8;
    if (fdi_battery_instrument_total_count >= 1000000) {
        fdi_adc_power_down();
    }
#else
    for (int i = 0; i < 8; ++i) {
        if (fdi_battery_instrument_sample_is_full()) {
            fdi_battery_instrument_sample_overflow = true;
            return;
        }

        uint16_t high_result = *results++;
        uint16_t low_result = *results++;
        double current_high = high_result * fdi_battery_instrument_multiplier_high;
        double current_low = low_result * fdi_battery_instrument_multiplier_low;
        double current = current_high > 0.000250 ? current_high : current_low;

        fdi_battery_instrument_sample.count += 1;
        double n = fdi_battery_instrument_sample.count;
        double mean = fdi_battery_instrument_sample.mean;
        double next_mean = mean + (current - mean) / n;
        fdi_battery_instrument_sample.S += (current - mean) * (current - next_mean);
        fdi_battery_instrument_sample.mean = next_mean;
        if (current < fdi_battery_instrument_sample.minimum) {
            fdi_battery_instrument_sample.minimum = current;
        }
        if (current > fdi_battery_instrument_sample.maximum) {
            fdi_battery_instrument_sample.maximum = current;
        }

        if (fdi_battery_instrument_sample.count >= fdi_battery_instrument_sample_decimation) {
            fdi_battery_instrument_sample_queue(fdi_battery_instrument_sample);
            fdi_battery_instrument_sample_clear(&fdi_battery_instrument_sample);

            fdi_battery_instrument_total_count += 1;
            if (fdi_battery_instrument_total_count >= fdi_battery_instrument_sample_total) {
                fdi_adc_power_down();
            }
        }
    }
#endif
}

void fdi_battery_instrument_sample_start(fdi_battery_instrument_t *instrument, uint32_t decimation, uint32_t total, uint32_t address) {
    fdi_battery_instrument_sample_decimation = decimation;
    fdi_battery_instrument_sample_total = total;
    fdi_battery_instrument_sample_address = address;

    fdi_battery_instrument_multiplier_high = (3.3 / 4096.0) * (double)instrument->multiplier_high;
    fdi_battery_instrument_multiplier_low = (3.3 / 4096.0) * (double)instrument->multiplier_low;

    fdi_battery_instrument_sample_clear(&fdi_battery_instrument_sample);

    fdi_battery_instrument_total_count = 0;

    fdi_battery_instrument_sample_head_index = 0;
    fdi_battery_instrument_sample_count = 0;
    fdi_battery_instrument_sample_overflow = false;

    fdi_adc_power_up();
    uint8_t channels[2] = { instrument->channel_high, instrument->channel_low };
    fdi_adc_convert_continuous(channels, sizeof(channels), fdi_battery_instrument_convert_continuous_callback);

    // !!! use timer to kick off each round of conversions at given rate? -denis
}

void fdi_battery_instrument_sample_stop(fdi_battery_instrument_t *instrument __attribute__((unused))) {
    fdi_adc_power_down();
}

void fdi_battery_instrument_save_conversions(void) {
    uint32_t count = fdi_battery_instrument_sample_count;
    if (count <= 0) {
        return;
    }

    for (; count != 0; --count) {
        uint32_t primask = fdi_interrupt_disable();
        fdi_battery_instrument_sample_t sample = fdi_battery_instrument_sample_dequeue();
        fdi_interrupt_restore(primask);
        
#if 0
        uint8_t buffer[16];
        fd_binary_t binary;
        fd_binary_initialize(&binary, buffer, sizeof(buffer));
        fd_binary_put_float32(&binary, sample.mean);
        fd_binary_put_float32(&binary, sample.minimum);
        fd_binary_put_float32(&binary, sample.maximum);
        float variance = sample.S / (sample.count - 1);
        fd_binary_put_float32(&binary, variance);
        fdi_s25fl116k_enable_write();
        fdi_s25fl116k_write_page(fdi_battery_instrument_sample_address, buffer, sizeof(buffer));
        fdi_battery_instrument_sample_address += sizeof(buffer);
#endif
    }

    if ((count == 0) && (fdi_battery_instrument_total_count >= fdi_battery_instrument_sample_total)) {
        uint8_t buffer[32];
        fd_binary_t response;
        fd_binary_initialize(&response, buffer, sizeof(buffer));
        fdi_battery_instrument_t *instrument = &fdi_battery_instruments[0]; // !!!
        if (!fdi_api_send(instrument->super.identifier, apiTypeConvertContinuousComplete, response.buffer, response.put_index)) {
            fd_log_assert_fail("can't send");
        }
    }
}

void fdi_battery_instrument_api_convert_continuous(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_battery_instrument_t *instrument = fdi_battery_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float rate = fd_binary_get_float32(binary);
    uint32_t decimation = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t total = (uint32_t)fd_binary_get_varuint(binary);
    uint32_t address = fd_binary_get_uint32(binary);

    if (rate >= 0.0f) {
        fdi_battery_instrument_sample_stop(instrument);
        fdi_battery_instrument_sample_start(instrument, decimation, total, address);
    } else {
        fdi_battery_instrument_sample_stop(instrument);
    }

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
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