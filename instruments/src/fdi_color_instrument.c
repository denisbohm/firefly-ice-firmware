#include "fdi_color_instrument.h"

#include "fdi_api.h"
#include "fdi_gpio.h"
#include "fdi_instrument.h"
#include "fdi_tcs3471.h"

#include "fd_binary.h"
#include "fd_log.h"

typedef struct {
    fdi_instrument_t super;
    uint8_t address;
} fdi_color_instrument_t;

#define fdi_color_instrument_count 1

static const uint64_t apiTypeConvert = 1;

fdi_color_instrument_t fdi_color_instruments[fdi_color_instrument_count];

fdi_color_instrument_t *fdi_color_instrument_get(uint64_t identifier) {
    for (int i = 0; i < fdi_color_instrument_count; ++i) {
        fdi_color_instrument_t *instrument = &fdi_color_instruments[i];
        if (instrument->super.identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}

void fdi_color_instrument_convert(uint64_t identifier, uint64_t type __attribute__((unused)), fd_binary_t *binary __attribute__((unused))) {
    fdi_color_instrument_t *instrument = fdi_color_instrument_get(identifier);
    if (instrument == 0) {
        return;
    }

    fdi_tcs3471_conversion_t conversion;
    fdi_tcs3471_convert(instrument->address, &conversion);

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
    fdi_instrument_register(&instrument->super);
    instrument->address = fdi_tcs34715_address;
    fdi_api_register(instrument->super.identifier, apiTypeConvert, fdi_color_instrument_convert);
}