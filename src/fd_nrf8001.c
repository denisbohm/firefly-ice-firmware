#include "fd_nrf8001.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_processor.h"
#include "fd_spi1.h"

#include <em_gpio.h>

void fd_nrf8001_initialize(void) {
}

void fd_nrf8001_reset(void) {
    // release reset
    GPIO_PinOutSet(NRF_RESETN_PORT_PIN);
    fd_delay_ms(100); // wait for nRF8001 to come out of reset (62ms)
}

void fd_nrf8001_transfer(void) {
    if (GPIO_PinInGet(NRF_RDYN_PORT_PIN)) {
        return;
    }

    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    fd_spi1_sync_transfer();
//    fd_spi1_start_transfer(0);
//    fd_spi1_wait();
    GPIO_PinOutSet(NRF_REQN_PORT_PIN);

    fd_spi1_tx_clear();

    uint8_t *buffer;
    uint32_t length;
    fd_spi1_get_rx(&buffer, &length);
    if (length <= 2) {
        return;
    }
    fd_nrf8001_dispatch(buffer + 2, length - 2);
}

static
void blocking_send(void) {
    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    while (GPIO_PinInGet(NRF_RDYN_PORT_PIN));
    fd_nrf8001_transfer();
}

void fd_nrf8001_send(uint8_t *message, uint32_t length) {
    fd_spi1_tx_clear();
    fd_spi1_tx_queue(message, length);
    fd_nrf8001_transfer();
    blocking_send();
}

void fd_nrf8001_send_with_data(uint8_t *message, uint32_t length, uint8_t *data, uint32_t data_length) {
    fd_spi1_tx_clear();
    fd_spi1_tx_queue(message, length);
    fd_spi1_tx_queue(data, data_length);
    blocking_send();
}