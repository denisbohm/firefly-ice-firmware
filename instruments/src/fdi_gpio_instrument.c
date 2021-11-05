#include "fdi_gpio_instrument.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_dac.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <string.h>

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeGetCapabilities = 1;
static const uint64_t apiTypeGetConfiguration = 2;
static const uint64_t apiTypeSetConfiguration = 3;
static const uint64_t apiTypeGetDigitalInput = 4;
static const uint64_t apiTypeSetDigitalOutput = 5;
static const uint64_t apiTypeGetAnalogInput = 6;
static const uint64_t apiTypeSetAnalogOutput = 7;
static const uint64_t apiTypeGetAuxiliaryConfiguration = 8;
static const uint64_t apiTypeSetAuxiliaryConfiguration = 9;
static const uint64_t apiTypeGetAuxiliaryInput = 10;
static const uint64_t apiTypeSetAuxiliaryOutput = 11;

static const fdi_gpio_instrument_setup_t fdi_gpio_instrument_setups[] = {

#ifdef FDI_INSTRUMENT_INPUT_OUTPUT
    { .gpio = FDI_GPIO_IOA0, .has_adc = true, .adc_channel = 5, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_AIO0, },
    { .gpio = FDI_GPIO_IOA1, .has_adc = true, .adc_channel = 6, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_AIO1, },
    { .gpio = FDI_GPIO_IOA2, .has_adc = true, .adc_channel = 7, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_AIO2, },
    { .gpio = FDI_GPIO_IOA3, .has_adc = true, .adc_channel = 8, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_AIO3, },
    { .gpio = FDI_GPIO_IOA4, .has_adc = true, .adc_channel = 9, .has_dac = true, .dac_channel = 4, },
    { .gpio = FDI_GPIO_IOA5, .has_adc = true, .adc_channel = 10, .has_dac = true, .dac_channel = 5, },
    { .gpio = FDI_GPIO_IOA6, .has_adc = true, .adc_channel = 11, },
    { .gpio = FDI_GPIO_IOA7, .has_adc = true, .adc_channel = 12, },
    { .gpio = FDI_GPIO_DIO0, },
    { .gpio = FDI_GPIO_DIO1, },
    { .gpio = FDI_GPIO_DIO2, },
    { .gpio = FDI_GPIO_DIO3, },
    { .gpio = FDI_GPIO_DIO4, },
    { .gpio = FDI_GPIO_DIO5, },
    { .gpio = FDI_GPIO_DIO6, },
    { .gpio = FDI_GPIO_DIO7, },
    { .gpio = FDI_GPIO_DIO8, },
    { .gpio = FDI_GPIO_DIO9, },
    { .gpio = FDI_GPIO_DIO10, },
    { .gpio = FDI_GPIO_DIO11, },
    { .gpio = FDI_GPIO_DIO12, },
    { .gpio = FDI_GPIO_DIO13, },
    { .gpio = FDI_GPIO_DIO14, },
    { .gpio = FDI_GPIO_DIO15, },
    { .gpio = FDI_GPIO_IOR0, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_RIO0, },
    { .gpio = FDI_GPIO_IOR1, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_RIO1, },
    { .gpio = FDI_GPIO_IOR2, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_RIO2, },
    { .gpio = FDI_GPIO_IOR3, .has_auxiliary = true, .auxiliary_gpio = FDI_GPIO_RIO3, },
#endif

#ifdef FDI_INSTRUMENT_SERIAL_WIRE
#endif

#ifdef FDI_INSTRUMENT_ALL_IN_ONE
#endif

};

#define fdi_gpio_instrument_count (sizeof(fdi_gpio_instrument_setups) / sizeof(fdi_gpio_instrument_setups[0]))
fdi_gpio_instrument_t fdi_gpio_instruments[fdi_gpio_instrument_count];

uint32_t fdi_gpio_instrument_get_count(void) {
    return fdi_gpio_instrument_count;
}

fdi_gpio_instrument_t *fdi_gpio_instrument_get_at(uint32_t index) {
    return &fdi_gpio_instruments[index];
}

fdi_gpio_instrument_t *fdi_gpio_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_gpio_instrument_count; ++i) {
        fdi_gpio_instrument_t *instrument = &fdi_gpio_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_gpio_instrument_reset(fdi_gpio_instrument_t *instrument) {
    if (instrument->setup->has_adc) {
        fdi_gpio_instrument_configuration_t configuration = {
            .domain = fdi_gpio_instrument_domain_analog,
        };
        fdi_gpio_instrument_set_configuration(instrument, &configuration);
    } else {
        if (instrument->setup->has_auxiliary) {
            fdi_gpio_instrument_configuration_t configuration = {
                .domain = fdi_gpio_instrument_domain_digital,
                .direction = fdi_gpio_instrument_direction_output,
            };
            fdi_gpio_instrument_set_configuration(instrument, &configuration);
        } else {
            fdi_gpio_instrument_configuration_t configuration = {
                .domain = fdi_gpio_instrument_domain_digital,
                .direction = fdi_gpio_instrument_direction_input,
            };
            fdi_gpio_instrument_set_configuration(instrument, &configuration);
        }
    }
    if (instrument->setup->has_auxiliary) {
        fdi_gpio_instrument_configuration_t configuration = {};
        fdi_gpio_instrument_set_auxiliary_configuration(instrument, &configuration);
    }
}

void fdi_gpio_instrument_api_reset(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_reset(instrument);
}

uint32_t fdi_gpio_instrument_get_capabilities(fdi_gpio_instrument_t *instrument) {
    uint32_t capabilities = 0;
    if (instrument->setup->has_adc) {
        capabilities |= 0x00000001;
    }
    if (instrument->setup->has_dac) {
        capabilities |= 0x00000002;
    }
    if (instrument->setup->has_auxiliary) {
        capabilities |= 0x00000004;
    }
    return capabilities;
}

void fdi_gpio_instrument_api_get_capabilities(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    uint32_t capabilities = fdi_gpio_instrument_get_capabilities(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint32(&response, capabilities);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetCapabilities, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_api_get_configuration(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_configuration_t configuration = instrument->configuration;

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, configuration.domain);
    fd_binary_put_uint8(&response, configuration.direction);
    fd_binary_put_uint8(&response, configuration.drive);
    fd_binary_put_uint8(&response, configuration.pull);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetConfiguration, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

static
void fdi_gpio_instrument_configure(
    fdi_gpio_instrument_t *instrument,
    uint32_t gpio,
    const fdi_gpio_instrument_configuration_t *configuration
) {
    switch (configuration->domain) {
        case fdi_gpio_instrument_domain_digital:
            switch (configuration->direction) {
                case fdi_gpio_instrument_direction_input:
                    fdi_gpio_set_mode(gpio, fdi_gpio_mode_input);
                    break;
                case fdi_gpio_instrument_direction_output:
                    fdi_gpio_set_mode(gpio, fdi_gpio_mode_output);
                    break;
            }
            switch (configuration->drive) {
                case fdi_gpio_instrument_drive_push_pull:
                case fdi_gpio_instrument_drive_open_drain:
                break;
            }
            switch (configuration->pull) {
                case fdi_gpio_instrument_pull_none:
                case fdi_gpio_instrument_pull_up:
                case fdi_gpio_instrument_pull_down:
                break;
            }
        break;
        case fdi_gpio_instrument_domain_analog:
            switch (configuration->direction) {
                case fdi_gpio_instrument_direction_input:
                    fdi_adc_setup(instrument->setup->adc_channel);
                    break;
                case fdi_gpio_instrument_direction_output:
                    fdi_dac_setup(instrument->setup->dac_channel);
                    break;
            }
        break;
    }
}

void fdi_gpio_instrument_set_configuration(fdi_gpio_instrument_t *instrument, const fdi_gpio_instrument_configuration_t *configuration) {
    fdi_gpio_instrument_configure(instrument, instrument->setup->gpio, configuration);
    memcpy(&instrument->configuration, configuration, sizeof(fdi_gpio_instrument_configuration_t));
}

void fdi_gpio_instrument_api_set_configuration(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_configuration_t configuration;
    configuration.domain = fd_binary_get_uint8(binary);
    configuration.direction = fd_binary_get_uint8(binary);
    configuration.drive = fd_binary_get_uint8(binary);
    configuration.pull = fd_binary_get_uint8(binary);
    fdi_gpio_instrument_set_configuration(instrument, &configuration);
}

bool fdi_gpio_instrument_get_digital_input(fdi_gpio_instrument_t *instrument) {
    return fdi_gpio_get(instrument->setup->gpio);
}

void fdi_gpio_instrument_api_get_digital_input(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool bit = fdi_gpio_instrument_get_digital_input(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, bit);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetDigitalInput, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_set_digital_output(fdi_gpio_instrument_t *instrument, bool bit) {
    fdi_gpio_set(instrument->setup->gpio, bit);
}

void fdi_gpio_instrument_api_set_digital_output(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool bit = fd_binary_get_uint8(binary) != 0;
    fdi_gpio_instrument_set_digital_output(instrument, bit);
}

float fdi_gpio_instrument_get_analog_input(fdi_gpio_instrument_t *instrument) {
    return fdi_adc_convert(instrument->setup->adc_channel);
}

void fdi_gpio_instrument_api_get_analog_input(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fdi_gpio_instrument_get_analog_input(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, voltage);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetAnalogInput, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_set_analog_output(fdi_gpio_instrument_t *instrument, float voltage) {
    uint32_t value = (uint32_t)(voltage * (4095.0f / 3.3f));
    fdi_dac_set(instrument->setup->dac_channel, value);
}

void fdi_gpio_instrument_api_set_analog_output(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float voltage = fd_binary_get_float32(binary);
    fdi_gpio_instrument_set_analog_output(instrument, voltage);
}

void fdi_gpio_instrument_api_get_auxiliary_configuration(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_configuration_t configuration = instrument->auxiliary_configuration;

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, configuration.domain);
    fd_binary_put_uint8(&response, configuration.direction);
    fd_binary_put_uint8(&response, configuration.drive);
    fd_binary_put_uint8(&response, configuration.pull);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetAuxiliaryConfiguration, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_set_auxiliary_configuration(fdi_gpio_instrument_t *instrument, const fdi_gpio_instrument_configuration_t *configuration) {
    fdi_gpio_instrument_configure(instrument, instrument->setup->auxiliary_gpio, configuration);
    memcpy(&instrument->auxiliary_configuration, configuration, sizeof(fdi_gpio_instrument_configuration_t));
}

void fdi_gpio_instrument_api_set_auxiliary_configuration(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_configuration_t configuration;
    configuration.domain = fd_binary_get_uint8(binary);
    configuration.direction = fd_binary_get_uint8(binary);
    configuration.drive = fd_binary_get_uint8(binary);
    configuration.pull = fd_binary_get_uint8(binary);
    fdi_gpio_instrument_set_auxiliary_configuration(instrument, &configuration);
}

bool fdi_gpio_instrument_get_auxiliary_input(fdi_gpio_instrument_t *instrument) {
    return fdi_gpio_get(instrument->setup->auxiliary_gpio);
}

void fdi_gpio_instrument_api_get_auxiliary_input(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool bit = fdi_gpio_instrument_get_auxiliary_input(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, bit);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetAuxiliaryInput, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_set_auxiliary_output(fdi_gpio_instrument_t *instrument, bool bit) {
    fdi_gpio_set(instrument->setup->auxiliary_gpio, bit);
}

void fdi_gpio_instrument_api_set_auxiliary_output(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool bit = fd_binary_get_uint8(binary) != 0;
    fdi_gpio_instrument_set_auxiliary_output(instrument, bit);
}

void fdi_gpio_instrument_initialize(void) {
    for (int i = 0; i < fdi_gpio_instrument_count; ++i) {
        fdi_gpio_instrument_t *instrument = &fdi_gpio_instruments[i];
        instrument->super.category = "Gpio";
        instrument->super.reset = fdi_gpio_instrument_api_reset;
        instrument->setup = &fdi_gpio_instrument_setups[i];
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_gpio_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeGetCapabilities, fdi_gpio_instrument_api_get_capabilities);
        fdi_api_register(instrument->super.identifier, apiTypeGetConfiguration, fdi_gpio_instrument_api_get_configuration);
        fdi_api_register(instrument->super.identifier, apiTypeSetConfiguration, fdi_gpio_instrument_api_set_configuration);
        fdi_api_register(instrument->super.identifier, apiTypeGetDigitalInput, fdi_gpio_instrument_api_get_digital_input);
        fdi_api_register(instrument->super.identifier, apiTypeSetDigitalOutput, fdi_gpio_instrument_api_set_digital_output);
        fdi_api_register(instrument->super.identifier, apiTypeGetAnalogInput, fdi_gpio_instrument_api_get_analog_input);
        fdi_api_register(instrument->super.identifier, apiTypeSetAnalogOutput, fdi_gpio_instrument_api_set_analog_output);
        fdi_api_register(instrument->super.identifier, apiTypeGetAuxiliaryConfiguration, fdi_gpio_instrument_api_get_auxiliary_configuration);
        fdi_api_register(instrument->super.identifier, apiTypeSetAuxiliaryConfiguration, fdi_gpio_instrument_api_set_auxiliary_configuration);
        fdi_api_register(instrument->super.identifier, apiTypeGetAuxiliaryInput, fdi_gpio_instrument_api_get_auxiliary_input);
        fdi_api_register(instrument->super.identifier, apiTypeSetAuxiliaryOutput, fdi_gpio_instrument_api_set_auxiliary_output);
    }
}