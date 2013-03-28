#include "fd_nrf8001.h"
#include "fd_processor.h"
#include "fd_spi1.h"

#include <em_gpio.h>

#define DeviceStartedEvent 0x81

void fd_nrf8001_initialize(void) {
}

void fd_nrf8001_reset(void) {
    // release reset
    GPIO_PinOutSet(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100); // wait for nRF8001 to come out of reset (62ms)
}

void fd_nrf8001_device_started(uint8_t operating_mode, uint8_t hardware_error, uint8_t data_credit_available) {
}

void fd_nrf8001_device_started_event(uint8_t *buffer, uint32_t length) {
    if (length != 4) {
        // log diagnostic
        return;
    }
    uint8_t operating_mode = buffer[1];
    uint8_t hardware_error = buffer[2];
    uint8_t data_credit_available = buffer[3];
    fd_nrf8001_device_started(operating_mode, hardware_error, data_credit_available);
}

void fd_nrf8001_dispatch(uint8_t *buffer, uint32_t length) {
    uint8_t op = buffer[0];
    switch (op) {
        case DeviceStartedEvent:
            fd_nrf8001_device_started_event(buffer, length);
        break;
        default:
        break;
    }
}

void fd_nrf8001_transfer(void) {
    if (!GPIO_PinInGet(NRF_RDYN_PORT_PIN)) {
        return;
    }

    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    fd_spi1_start_transfer(0);
    fd_spi1_wait();
    GPIO_PinOutSet(NRF_REQN_PORT_PIN);

    uint8_t *buffer;
    uint32_t length;
    fd_spi1_get_rx(&buffer, &length);
    if (length <= 2) {
        return;
    }
    fd_nrf8001_dispatch(buffer + 2, length - 2);
}