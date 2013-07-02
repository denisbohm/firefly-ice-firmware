#ifndef FD_SPI0_H
#define FD_SPI0_H

#include <stdint.h>

void fd_spi0_initialize(void);
void fd_spi0_sleep(void);
void fd_spi0_wake(void);
void fd_spi0_power_on(void);
void fd_spi0_power_off(void);
void fd_spi0_tx_clear(void);
void fd_spi0_tx_queue(uint8_t *buffer, uint32_t length);
void fd_spi0_start_transfer(void (*callback)(void));
void fd_spi0_wait(void);
void fd_spi0_sync_transfer(void);
void fd_spi0_get_rx(uint8_t **pbuffer, uint32_t *plength);
void fd_spi0_rx_clear(void);

#endif