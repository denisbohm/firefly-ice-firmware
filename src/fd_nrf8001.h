#ifndef FD_NRF8001_H
#define FD_NRF8001_H

#include <stdbool.h>
#include <stdint.h>

void fd_nrf8001_initialize(void);

void fd_nrf8001_reset(void);

void fd_nrf8001_transfer(void);

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

#endif