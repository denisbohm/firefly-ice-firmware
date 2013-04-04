#ifndef FD_NRF8001_H
#define FD_NRF8001_H

#include <stdint.h>

void fd_nrf8001_initialize(void);

void fd_nrf8001_reset(void);

void fd_nrf8001_transfer(void);

void fd_nrf8001_send(uint8_t *message, uint32_t length);

void fd_nrf8001_send_with_data(uint8_t *message, uint32_t length, uint8_t *data, uint32_t data_length);

#endif