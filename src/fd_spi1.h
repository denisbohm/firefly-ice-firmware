#ifndef FD_SPI1_H
#define FD_SPI1_H

#include <stdint.h>

void fd_spi1_initialize(void);
void fd_spi1_sleep(void);
void fd_spi1_wake(void);
void fd_spi1_power_on(void);
void fd_spi1_power_off(void);
void fd_spi1_tx_clear(void);
void fd_spi1_tx_queue(uint8_t *buffer, uint32_t length);
void fd_spi1_start_transfer(void (*callback)(void));
void fd_spi1_wait(void);
void fd_spi1_sync_transfer(void);
void fd_spi1_get_rx(uint8_t **pbuffer, uint32_t *plength);
void fd_spi1_rx_clear(void);

#endif