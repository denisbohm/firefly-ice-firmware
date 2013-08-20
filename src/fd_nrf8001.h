#ifndef FD_NRF8001_H
#define FD_NRF8001_H

#include <stdbool.h>
#include <stdint.h>

void fd_nrf8001_initialize(void);

bool fd_nrf8001_has_system_credits(void);
void fd_nrf8001_set_system_credits(uint32_t credits);
void fd_nrf8001_add_system_credits(uint32_t credits);
void fd_nrf8001_use_system_credits(uint32_t credits);

bool fd_nrf8001_has_data_credits(void);
void fd_nrf8001_set_data_credits(uint32_t credits);
void fd_nrf8001_add_data_credits(uint32_t credits);
void fd_nrf8001_use_data_credits(uint32_t credits);

void fd_nrf8001_send(uint8_t *message, uint32_t length);

void fd_nrf8001_send_with_data(uint8_t *message, uint32_t length, uint8_t *data, uint32_t data_length);

void fd_nrf8001_error(void);


void fd_nrf8001_spi_tx_clear(void);

#define FD_NRF8001_SPI_TX_BUFFER_SIZE 32

extern uint8_t fd_nrf8001_spi_tx_buffer[FD_NRF8001_SPI_TX_BUFFER_SIZE];
extern uint32_t fd_nrf8001_spi_tx_length;

#define FD_NRF8001_SPI_RX_BUFFER_SIZE 32

void fd_nrf8001_spi_transfer(void);

#endif