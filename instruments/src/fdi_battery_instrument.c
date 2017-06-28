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

#define fdi_battery_instrument_sample_decimation_limit 1000

uint32_t fdi_battery_instrument_sample_decimation;
uint32_t fdi_battery_instrument_sample_total;
uint32_t fdi_battery_instrument_sample_address;

float fdi_battery_instrument_multiplier_high;
float fdi_battery_instrument_multiplier_low;

volatile uint16_t fdi_battery_instrument_buffer_0[2 * fdi_battery_instrument_sample_decimation_limit] = {0xAAAA};
volatile uint16_t fdi_battery_instrument_buffer_1[2 * fdi_battery_instrument_sample_decimation_limit] = {0xAAAA};

static uint32_t conversion_count;
static uint32_t conversion_high_result_sum;
static uint32_t conversion_low_result_sum;

#define fdi_battery_instrument_sample_clear() do {\
    conversion_count = 0;\
    conversion_high_result_sum = 0;\
    conversion_low_result_sum = 0;\
} while (0)

uint32_t fdi_battery_instrument_total_count;

#define fdi_battery_instrument_sample_size fdi_battery_instrument_sample_decimation_limit
volatile float fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_size];
volatile uint32_t fdi_battery_instrument_sample_head_index;
volatile uint32_t fdi_battery_instrument_sample_count;
volatile bool fdi_battery_instrument_sample_overflow;

#define fdi_battery_instrument_sample_is_empty() (fdi_battery_instrument_sample_count == 0)

#define fdi_battery_instrument_sample_is_full() (fdi_battery_instrument_sample_count == fdi_battery_instrument_sample_size)

#define fdi_battery_instrument_sample_has_room(n) ((fdi_battery_instrument_sample_count + (n)) <= fdi_battery_instrument_sample_size)

#define fdi_battery_instrument_sample_queue(value) do {\
    fdi_battery_instrument_sample_buffer[fdi_battery_instrument_sample_head_index] = (value);\
    if (++fdi_battery_instrument_sample_head_index >= fdi_battery_instrument_sample_size) {\
        fdi_battery_instrument_sample_head_index = 0;\
    }\
    ++fdi_battery_instrument_sample_count;\
} while (0)

float fdi_battery_instrument_sample_dequeue(void) {
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

void fdi_battery_instrument_convert_continuous_callback(volatile uint16_t *results, uint32_t size __attribute__((unused))) {
    if (fdi_battery_instrument_sample_is_full()) {
        fdi_battery_instrument_sample_overflow = true;
        return;
    }

    volatile uint16_t *end = &results[fdi_battery_instrument_sample_decimation * 2];
    while (results < end) {
        uint16_t high_result = *results++;
        uint16_t low_result = *results++;
        if (low_result < 0xff0) {
            conversion_low_result_sum += low_result;
        } else {
            conversion_high_result_sum += high_result;
        }
    }

    float value =
        conversion_high_result_sum * fdi_battery_instrument_multiplier_high +
        conversion_low_result_sum * fdi_battery_instrument_multiplier_low;
    fdi_battery_instrument_sample_queue(value);
    fdi_battery_instrument_sample_clear();

    if (++fdi_battery_instrument_total_count >= fdi_battery_instrument_sample_total) {
        fdi_adc_power_down();
    }
}

void fdi_battery_instrument_sample_start(fdi_battery_instrument_t *instrument, uint32_t decimation, uint32_t total, uint32_t address) {
    fdi_battery_instrument_sample_decimation = decimation;
    fdi_battery_instrument_sample_total = total;
    fdi_battery_instrument_sample_address = address;

    fdi_battery_instrument_multiplier_high = (3.3f / 4096.0f) * instrument->multiplier_high / (float)decimation;
    fdi_battery_instrument_multiplier_low = (3.3f / 4096.0f) * instrument->multiplier_low / (float)decimation;

    fdi_battery_instrument_sample_clear();

    fdi_battery_instrument_total_count = 0;

    fdi_battery_instrument_sample_head_index = 0;
    fdi_battery_instrument_sample_count = 0;
    fdi_battery_instrument_sample_overflow = false;

    fdi_adc_power_up();
    uint8_t channels[2] = { instrument->channel_high, instrument->channel_low };
    fdi_adc_convert_continuous(channels, sizeof(channels), fdi_battery_instrument_buffer_0, fdi_battery_instrument_buffer_1, decimation, fdi_battery_instrument_convert_continuous_callback);
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
        float value = fdi_battery_instrument_sample_dequeue();
        fdi_interrupt_restore(primask);
        
        uint8_t buffer[4];
        fd_binary_t binary;
        fd_binary_initialize(&binary, buffer, sizeof(buffer));
        fd_binary_put_float32(&binary, value);
        fdi_s25fl116k_enable_write();
        fdi_s25fl116k_write_page(fdi_battery_instrument_sample_address, buffer, sizeof(buffer));
        fdi_battery_instrument_sample_address += sizeof(buffer);
    }

    if ((count == 0) && (fdi_battery_instrument_total_count >= fdi_battery_instrument_sample_total)) {
        fdi_s25fl116k_wait_while_busy();

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

    uint64_t result = 0;
    if (decimation > 1000) {
        result = 1; // error invalid parameter decimation
    } else {
        if (rate >= 0.0f) {
            fdi_battery_instrument_sample_stop(instrument);
            fdi_battery_instrument_sample_start(instrument, decimation, total, address);
        } else {
            fdi_battery_instrument_sample_stop(instrument);
        }
    }

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_varuint(&response, result);
    if (!fdi_api_send(instrument->super.identifier, apiTypeConvertContinuous, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

/*
The output voltage of the LDO (VOUT) can be calculated by noting the following:
VOUT = VREF + i1R1	(Eq. 1)
i1 = i2 + i3	(Eq. 2)
i2 = VREF/R2	(Eq. 3)
i3 = (VREF - VDAC)/R3	(Eq. 4)
Substituting Equations 2 through 4 into Equation 1 yields:
VOUT = VREF(1 + (R1/R2)) + (VREF - VDAC) (R1/R3)	(Eq. 5)
*/
void fdi_battery_instrument_set_voltage(fdi_battery_instrument_t *instrument __attribute__((unused)), float voltage) {
    if (voltage < 2.75f) {
        voltage = 2.75f;
    }
    if (voltage > 4.2f) {
        voltage = 4.2f;
    }
    // 12-bit ADC w/ 3.3V VDD reference, 0.4 V Reference in LDO, 650k over 100k (3 V)
    // 0x0222 = 546 = 2.75 V (battery cut-off) = 0.440 V ADC output
    // 0x010d = 269 = 4.2 V (max value) = 0.217 V ADC output
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