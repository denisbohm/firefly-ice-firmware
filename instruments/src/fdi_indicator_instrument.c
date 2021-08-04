#include "fdi_indicator_instrument.h"

#include "fdi_api.h"
#ifdef FDI_INSTRUMENT_ALL_IN_ONE
#include "fdi_gpio.h"
#endif
#include "fdi_instrument.h"

#include "fd_binary.h"

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeSetRGB = 1;

#define fdi_indicator_instrument_count 1
fdi_indicator_instrument_t fdi_indicator_instruments[fdi_indicator_instrument_count];

uint32_t fdi_indicator_instrument_get_count(void) {
    return fdi_indicator_instrument_count;
}

fdi_indicator_instrument_t *fdi_indicator_instrument_get_at(uint32_t index) {
    return &fdi_indicator_instruments[index];
}

fdi_indicator_instrument_t *fdi_indicator_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_indicator_instrument_count; ++i) {
        fdi_indicator_instrument_t *instrument = &fdi_indicator_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_indicator_instrument_reset(fdi_indicator_instrument_t *instrument) {
#ifdef FDI_INSTRUMENT_SERIAL_WIRE
    fdi_indicator_instrument_set_rgb(instrument, 0.0f, 0.0f, 0.0f);
#endif
#ifdef FDI_INSTRUMENT_ALL_IN_ONE
    fdi_gpio_on(instrument->led_r); 
    fdi_gpio_on(instrument->led_g); 
    fdi_gpio_on(instrument->led_b); 
#endif
}

void fdi_indicator_instrument_api_reset(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary __attribute((unused))) {
    fdi_indicator_instrument_t *instrument = fdi_indicator_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_indicator_instrument_reset(instrument); 
}

void fdi_indicator_instrument_set_rgb(fdi_indicator_instrument_t *instrument, float r, float g, float b) {
#ifdef FDI_INSTRUMENT_SERIAL_WIRE
#endif
#ifdef FDI_INSTRUMENT_ALL_IN_ONE
    fdi_gpio_set(instrument->led_r, r < 0.5f); 
    fdi_gpio_set(instrument->led_g, g < 0.5f); 
    fdi_gpio_set(instrument->led_b, b < 0.5f);
#endif
}

void fdi_indicator_instrument_api_set_rgb(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_indicator_instrument_t *instrument = fdi_indicator_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float r = fd_binary_get_float32(binary);
    float g = fd_binary_get_float32(binary);
    float b = fd_binary_get_float32(binary);

    fdi_indicator_instrument_set_rgb(instrument, r, g, b); 
}

void fdi_indicator_instrument_initialize(void) {
    fdi_indicator_instrument_t *instrument = &fdi_indicator_instruments[0];
    instrument->super.category = "Indicator";
    instrument->super.reset = fdi_indicator_instrument_api_reset;
#ifdef FDI_INSTRUMENT_ALL_IN_ONE
    instrument->led_r = FDI_GPIO_LED_R;
    instrument->led_g = FDI_GPIO_LED_G;
    instrument->led_b = FDI_GPIO_LED_B;
#endif
    fdi_instrument_register(&instrument->super);
    fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_indicator_instrument_api_reset);
    fdi_api_register(instrument->super.identifier, apiTypeSetRGB, fdi_indicator_instrument_api_set_rgb);
}