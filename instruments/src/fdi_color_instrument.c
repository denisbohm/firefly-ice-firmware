#include "fdi_color_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_tcs3471.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <math.h>

static const uint64_t apiTypeReset = 0;
static const uint64_t apiTypeConvert = 1;

#define fdi_color_instrument_count 1
fdi_color_instrument_t fdi_color_instruments[fdi_color_instrument_count];

uint32_t fdi_color_instrument_get_count(void) {
    return fdi_color_instrument_count;
}

fdi_color_instrument_t *fdi_color_instrument_get_at(uint32_t index) {
    return &fdi_color_instruments[index];
}

fdi_color_instrument_t *fdi_color_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_color_instrument_count; ++i) {
        fdi_color_instrument_t *instrument = &fdi_color_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_color_instrument_reset(fdi_color_instrument_t *instrument __attribute__((unused))) {
    // nothing to do...
}

void fdi_color_instrument_api_reset(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_color_instrument_t *instrument = fdi_color_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_color_instrument_reset(instrument);
}

fdi_tcs3471_conversion_t fdi_color_instrument_convert(fdi_color_instrument_t *instrument, float integration_time, float gain) {
    float atimef = 256.0f - integration_time / 0.0024f;
    if (atimef < 0.0f) {
        atimef = 0.0f;
    }
    if (atimef > 256.0f) {
        atimef = 256.0f;
    }
    uint8_t atime = (uint8_t)roundf(atimef);

    uint8_t again;
    if (gain <= 1.0f) {
        again = 0b00;
    } else
    if (gain <= 4.0f) {
        again = 0b01;
    } else
    if (gain <= 16.0f) {
        again = 0b10;
    } else {
        again = 0b11;
    }

    fdi_tcs3471_conversion_t conversion;
    fdi_tcs3471_convert(instrument->address, atime, again, &conversion);
    return conversion;
}

void fdi_color_instrument_api_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary) {
    fdi_color_instrument_t *instrument = fdi_color_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    float integration_time = fd_binary_get_float32(binary);
    float gain = fd_binary_get_float32(binary);

    fdi_tcs3471_conversion_t conversion = fdi_color_instrument_convert(instrument, integration_time, gain);

    uint8_t buffer[32];
    fd_binary_t response;
    fd_binary_initialize(&response, buffer, sizeof(buffer));
    fd_binary_put_float32(&response, (float)conversion.c);
    fd_binary_put_float32(&response, (float)conversion.r);
    fd_binary_put_float32(&response, (float)conversion.g);
    fd_binary_put_float32(&response, (float)conversion.b);

    if (!fdi_api_send(instrument->super.identifier, apiTypeConvert, response.buffer, response.put_index)) {
        fd_log_assert_fail("can't send");
    }
}

void fdi_color_instrument_initialize(void) {
    fdi_color_instrument_t *instrument = &fdi_color_instruments[0];
    instrument->super.category = "Color";
    instrument->super.reset = fdi_color_instrument_api_reset;
    instrument->address = fdi_tcs34715_address;
    fdi_instrument_register(&instrument->super);
    fdi_api_register(instrument->super.identifier, apiTypeReset, fdi_color_instrument_api_reset);
    fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_color_instrument_api_convert);
}