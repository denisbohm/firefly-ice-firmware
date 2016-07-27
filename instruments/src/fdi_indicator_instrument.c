#include "fdi_indicator_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"

#include "fd_binary.h"

typedef struct {
    fdi_instrument_t super;
    uint32_t led_r;
    uint32_t led_g;
    uint32_t led_b;
} fdi_indicator_instrument_t;

#define fdi_indicator_instrument_count 1

static const uint64_t apiTypeSetRGB = 1;

fdi_indicator_instrument_t fdi_indicator_instruments[fdi_indicator_instrument_count];

fdi_indicator_instrument_t *fdi_indicator_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_indicator_instrument_count; ++i) {
        fdi_indicator_instrument_t *instrument = &fdi_indicator_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_indicator_instrument_set_rgb(uint64_t identifier, uint64_t type __attribute((unused)), fd_binary_t *binary) {
    fdi_indicator_instrument_t *instrument = fdi_indicator_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float r = fd_binary_get_float32(binary);
    float g = fd_binary_get_float32(binary);
    float b = fd_binary_get_float32(binary);
    fdi_gpio_set(instrument->led_r, r < 0.5f); 
    fdi_gpio_set(instrument->led_g, g < 0.5f); 
    fdi_gpio_set(instrument->led_b, b < 0.5f); 
}

void fdi_indicator_instrument_initialize(void) {
    fdi_indicator_instrument_t *instrument = &fdi_indicator_instruments[0];
    instrument->super.category = "Indicator";
    fdi_instrument_register(&instrument->super);
    instrument->led_r = FDI_GPIO_LED_R;
    instrument->led_g = FDI_GPIO_LED_G;
    instrument->led_b = FDI_GPIO_LED_B;
    fdi_api_register(instrument->super.identifier, apiTypeSetRGB, fdi_indicator_instrument_set_rgb);
}