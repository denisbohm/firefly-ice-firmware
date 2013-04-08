#include "fd_log.h"
#include "fd_nrf8001.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_processor.h"
#include "fd_spi1.h"

#include <em_gpio.h>

static uint32_t system_credits;
static uint32_t data_credits;

void fd_nrf8001_initialize(void) {
    system_credits = 0;
    data_credits = 0;
}

void fd_nrf8001_error(void) {
}

bool fd_nrf8001_has_system_credits(void) {
    return system_credits > 0;
}

void fd_nrf8001_set_system_credits(uint32_t credits) {
    system_credits = credits;
}

void fd_nrf8001_add_system_credits(uint32_t credits) {
    system_credits += credits;

    if (system_credits > 1) {
        fd_log("");
        fd_nrf8001_error();
        system_credits = 1;
    }
}

void fd_nrf8001_use_system_credits(uint32_t credits) {
    if (credits > system_credits) {
        fd_log("");
        fd_nrf8001_error();
        system_credits = 0;
        return;
    }

    system_credits -= credits;
}

bool fd_nrf8001_has_data_credits(void) {
    return data_credits > 0;
}

void fd_nrf8001_set_data_credits(uint32_t credits) {
    data_credits = credits;
}

void fd_nrf8001_add_data_credits(uint32_t credits) {
    data_credits += credits;

    if (data_credits > 2) {
        fd_log("");
        fd_nrf8001_error();
        data_credits = 2;
    }
}

void fd_nrf8001_use_data_credits(uint32_t credits) {
    if (credits > data_credits) {
        fd_log("");
        fd_nrf8001_error();
        data_credits = 0;
        return;
    }

    data_credits -= credits;
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