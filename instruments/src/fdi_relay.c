#include "fdi_relay.h"

#include "fd_log.h"

void fdi_relay_initialize(void) {
}

void fdi_relay_set(uint32_t identifier, bool value) {
    if (value) {
        fdi_relay_on(identifier);
    } else {
        fdi_relay_off(identifier);
    }
}

void fdi_relay_on(uint32_t identifier) {
    fdi_gpio_on(identifier);
}

void fdi_relay_off(uint32_t identifier) {
    fdi_gpio_off(identifier);
}
