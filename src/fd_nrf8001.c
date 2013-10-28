#include "fd_nrf8001.h"

#include <string.h>

static uint32_t system_credits;
static uint32_t data_credits;

uint8_t fd_nrf8001_spi_tx_buffer[FD_NRF8001_SPI_TX_BUFFER_SIZE];
uint32_t fd_nrf8001_spi_tx_length;

void fd_nrf8001_initialize(void) {
    system_credits = 0;
    data_credits = 0;

    fd_nrf8001_spi_tx_length = 0;
    for (int i = 0; i < FD_NRF8001_SPI_TX_BUFFER_SIZE; ++i) {
        fd_nrf8001_spi_tx_buffer[i] = 0;
    }
}

#define WEAK __attribute__((weak))
// command responses

bool fd_nrf8001_has_system_credits(void) {
    return system_credits > 0;
}

WEAK
void fd_nrf8001_system_credit_change(void) {
}

void fd_nrf8001_set_system_credits(uint32_t credits) {
    system_credits = credits;

    fd_nrf8001_system_credit_change();
}

void fd_nrf8001_add_system_credits(uint32_t credits) {
    system_credits += credits;

    if (system_credits > 1) {
        fd_nrf8001_error();
        system_credits = 1;
    }

    fd_nrf8001_system_credit_change();
}

void fd_nrf8001_use_system_credits(uint32_t credits) {
    if (credits > system_credits) {
        fd_nrf8001_error();
        system_credits = 0;
        return;
    }

    system_credits -= credits;

    fd_nrf8001_system_credit_change();
}

bool fd_nrf8001_has_data_credits(void) {
    return data_credits > 0;
}

WEAK
void fd_nrf8001_data_credit_change(void) {
}

void fd_nrf8001_set_data_credits(uint32_t credits) {
    data_credits = credits;

    fd_nrf8001_data_credit_change();
}

void fd_nrf8001_add_data_credits(uint32_t credits) {
    data_credits += credits;

    if (data_credits > 2) {
        fd_nrf8001_error();
        data_credits = 2;
    }

    fd_nrf8001_data_credit_change();
}

void fd_nrf8001_use_data_credits(uint32_t credits) {
    if (credits > data_credits) {
        fd_nrf8001_error();
        data_credits = 0;
        return;
    }

    data_credits -= credits;

    fd_nrf8001_data_credit_change();
}

void fd_nrf8001_spi_tx_clear(void) {
    fd_nrf8001_spi_tx_length = 0;
}

static
void fd_nrf8001_spi_tx_queue(uint8_t *data, uint32_t length) {
    if ((fd_nrf8001_spi_tx_length + length) > FD_NRF8001_SPI_TX_BUFFER_SIZE) {
        fd_nrf8001_error();
        return;
    }

    memcpy(&fd_nrf8001_spi_tx_buffer[fd_nrf8001_spi_tx_length], data, length);
    fd_nrf8001_spi_tx_length += length;
}

void fd_nrf8001_send(uint8_t *message, uint32_t length) {
    if (fd_nrf8001_spi_tx_length != 0) {
        fd_nrf8001_error();
    }
    fd_nrf8001_spi_tx_clear();
    fd_nrf8001_spi_tx_queue(message, length);
    fd_nrf8001_spi_transfer();
}

void fd_nrf8001_send_with_data(uint8_t *message, uint32_t length, uint8_t *data, uint32_t data_length) {
    if (fd_nrf8001_spi_tx_length != 0) {
        fd_nrf8001_error();
    }
    fd_nrf8001_spi_tx_clear();
    fd_nrf8001_spi_tx_queue(message, length);
    fd_nrf8001_spi_tx_queue(data, data_length);
    fd_nrf8001_spi_transfer();
}
