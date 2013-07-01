#ifndef FD_SPI0_H
#define FD_SPI0_H

#include <stdint.h>

#include <em_gpio.h>

void fd_spi0_initialize(void);

void fd_spi0_sleep(void);
void fd_spi0_wake(void);

typedef enum {fd_spi_op_read = 0x01, fd_spi_op_write = 0x02, fd_spi_op_read_write = 0x03} fd_spi_op;

typedef struct {
    fd_spi_op op;
    uint32_t length;
    uint8_t *tx_buffer;
    uint8_t *rx_buffer;
} fd_spi_transfer;

void fd_spi0_io(GPIO_Port_TypeDef csn_port, unsigned int csn_pin, fd_spi_transfer *tranfers, uint32_t count, void (*callback)(void));

void fd_spi0_wait(void);

// synchronous call that writes the address and reads a 1-byte result using async I/O
uint8_t fd_spi0_read(GPIO_Port_TypeDef csn_port, unsigned int csn_pin, uint8_t address);

uint8_t fd_spi0_sync_io(uint8_t txdata);
uint8_t fd_spi0_sync_read(uint8_t address);
void fd_spi0_sync_write(uint8_t address, uint8_t value);
void fd_spi0_sync_read_bytes(uint8_t address, uint8_t *bytes, uint32_t length);

#endif