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
    fd_log_assert((FDI_RELAY_ATE_USB_5V_EN <= identifier) && (identifier < FDI_RELAY_ATE_MCU_VCC_SENSE));

    fdi_gpio_on(identifier);
}

void fdi_relay_off(uint32_t identifier) {
    fd_log_assert((FDI_RELAY_ATE_USB_5V_EN <= identifier) && (identifier < FDI_RELAY_ATE_MCU_VCC_SENSE));

    fdi_gpio_off(identifier);
}
