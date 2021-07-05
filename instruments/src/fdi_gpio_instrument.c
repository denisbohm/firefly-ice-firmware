#include "fdi_gpio_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"

#include "fd_binary.h"
#include "fd_log.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeSetBit = 1;
static const uint64_t apiTypeGetBit = 2;
static const uint64_t apiTypeSetModeBit = 3;

static const uint32_t fdi_gpio_instrument_controls[] = {

#ifdef FDI_INSTRUMENT_INPUT_OUTPUT
    FDI_GPIO_DIO0,
    FDI_GPIO_DIO1,
    FDI_GPIO_DIO2,
    FDI_GPIO_DIO3,
    FDI_GPIO_DIO4,
    FDI_GPIO_DIO5,
    FDI_GPIO_DIO6,
    FDI_GPIO_DIO7,
    FDI_GPIO_DIO8,
    FDI_GPIO_DIO9,
    FDI_GPIO_DIO10,
    FDI_GPIO_DIO11,
    FDI_GPIO_DIO12,
    FDI_GPIO_DIO13,
    FDI_GPIO_DIO14,
    FDI_GPIO_DIO15,
    FDI_GPIO_RIO0,
    FDI_GPIO_RIO1,
    FDI_GPIO_RIO2,
    FDI_GPIO_RIO3,
#endif

#ifdef FDI_INSTRUMENT_SERIAL_WIRE
#endif

#ifdef FDI_INSTRUMENT_ALL_IN_ONE
#endif

};

#define fdi_gpio_instrument_count (sizeof(fdi_gpio_instrument_controls) / sizeof(fdi_gpio_instrument_controls[0]))
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
    fdi_gpio_set_mode_input(instrument->control);
    fdi_gpio_off(instrument->control);
}

void fdi_gpio_instrument_api_reset(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_instrument_reset(instrument);
}

void fdi_gpio_instrument_set_bit(fdi_gpio_instrument_t *instrument, bool on) {
    fdi_gpio_set(instrument->control, on);
}

void fdi_gpio_instrument_api_set_bit(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool on = fd_binary_get_uint8(binary) != 0;
    fdi_gpio_instrument_set_bit(instrument, on);
}

void fdi_gpio_instrument_set_mode_bit(fdi_gpio_instrument_t *instrument, fdi_gpio_mode_t mode) {
    fdi_gpio_set_mode(instrument->control, mode);
}

void fdi_gpio_instrument_api_set_mode_bit(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_gpio_mode_t mode = fd_binary_get_uint8(binary);
    fdi_gpio_instrument_set_mode_bit(instrument, mode);
}

bool fdi_gpio_instrument_get_bit(fdi_gpio_instrument_t *instrument) {
    return fdi_gpio_get(instrument->control);
}

void fdi_gpio_instrument_api_get_bit(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_gpio_instrument_t *instrument = fdi_gpio_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    bool bit = fdi_gpio_instrument_get_bit(instrument);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_uint8(&response, bit);

    if (!fdi_api_send(instrument->super.identifier, apiTypeGetBit, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_gpio_instrument_initialize(void) {
    for (int i = 0; i < fdi_gpio_instrument_count; ++i) {
        fdi_gpio_instrument_t *instrument = &fdi_gpio_instruments[i];
        instrument->super.category = "Gpio";
        instrument->super.reset = fdi_gpio_instrument_api_reset;
        instrument->control = fdi_gpio_instrument_controls[i];
        fdi_instrument_register(&instrument->super);
        fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_gpio_instrument_api_reset);
        fdi_api_register(instrument->super.identifier, apiTypeSetBit, fdi_gpio_instrument_api_set_bit);
        fdi_api_register(instrument->super.identifier, apiTypeGetBit, fdi_gpio_instrument_api_get_bit);
        fdi_api_register(instrument->super.identifier, apiTypeSetModeBit, fdi_gpio_instrument_api_set_mode_bit);
    }
}