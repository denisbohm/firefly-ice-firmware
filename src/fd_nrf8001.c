#include "fd_log.h"
#include "fd_nrf8001.h"
#include "fd_nrf8001_dispatch.h"
#include "fd_processor.h"
#include "fd_spi.h"

#include <em_gpio.h>

#include <string.h>

static uint32_t system_credits;
static uint32_t data_credits;

#define FD_NRF8001_SPI_TX_BUFFER_SIZE 32

static uint8_t fd_nrf8001_spi_tx_buffer[FD_NRF8001_SPI_TX_BUFFER_SIZE];
static uint32_t fd_nrf8001_spi_tx_length;

void fd_nrf8001_initialize(void) {
    system_credits = 0;
    data_credits = 0;

    fd_nrf8001_spi_tx_length = 0;
}

void fd_nrf8001_error(void) {
    fd_log_assert_fail("");
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

static
void fd_nrf8001_spi_tx_clear(void) {
    fd_nrf8001_spi_tx_length = 0;
}

static
void fd_nrf8001_spi_tx_queue(uint8_t *data, uint32_t length) {
    if ((fd_nrf8001_spi_tx_length + length) > FD_NRF8001_SPI_TX_BUFFER_SIZE) {
        fd_log_assert_fail("");
        return;
    }

    memcpy(&fd_nrf8001_spi_tx_buffer[fd_nrf8001_spi_tx_length], data, length);
    fd_nrf8001_spi_tx_length += length;
}

#define FD_NRF8001_SPI_RX_BUFFER_SIZE 32

void fd_nrf8001_transfer(void) {
    if (GPIO_PinInGet(NRF_RDYN_PORT_PIN)) {
        return;
    }

    uint8_t rx_buffer[FD_NRF8001_SPI_RX_BUFFER_SIZE];
    fd_spi_transfer_t transfers[] = {
        {
            .op = fd_nrf8001_spi_tx_length ? fd_spi_op_read_write : fd_spi_op_read,

            .tx_buffer = fd_nrf8001_spi_tx_buffer,
            .tx_size = FD_NRF8001_SPI_TX_BUFFER_SIZE,
            .tx_length = fd_nrf8001_spi_tx_length,

            .rx_buffer = rx_buffer,
            .rx_size = FD_NRF8001_SPI_RX_BUFFER_SIZE,
            .rx_length = 2,
        },
    };
    fd_spi_io_t io = {
        .options = FD_SPI_OPTION_VARLEN,
        .transfers = transfers,
        .transfers_count = sizeof(transfers) / sizeof(fd_spi_transfer_t),
        .completion_callback = 0,
    };
    fd_spi_io(FD_SPI_BUS_1_SLAVE_NRF8001, &io);
    fd_spi_wait(FD_SPI_BUS_1);

    fd_nrf8001_spi_tx_clear();

    uint32_t length = rx_buffer[1];
    if (length > 0) {
        fd_nrf8001_dispatch(&rx_buffer[2], length);
    }
}

static
void blocking_send(void) {
    GPIO_PinOutClear(NRF_REQN_PORT_PIN);
    while (GPIO_PinInGet(NRF_RDYN_PORT_PIN));
    fd_nrf8001_transfer();
}

void fd_nrf8001_send(uint8_t *message, uint32_t length) {
    fd_nrf8001_spi_tx_clear();
    fd_nrf8001_spi_tx_queue(message, length);
    fd_nrf8001_transfer();
    blocking_send();
}

void fd_nrf8001_send_with_data(uint8_t *message, uint32_t length, uint8_t *data, uint32_t data_length) {
    fd_nrf8001_spi_tx_clear();
    fd_nrf8001_spi_tx_queue(message, length);
    fd_nrf8001_spi_tx_queue(data, data_length);
    blocking_send();
}