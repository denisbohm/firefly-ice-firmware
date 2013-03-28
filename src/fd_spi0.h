#ifndef FD_SPI0_H
#define FD_SPI0_H

#include <stdint.h>

void fd_spi0_initialize(void);

typedef enum {fd_spi_op_read = 0x01, fd_spi_op_write = 0x02, fd_spi_op_read_write = 0x03} fd_spi_op;

typedef struct {
    fd_spi_op op;
    uint32_t length;
    uint8_t *tx_buffer;
    uint8_t *rx_buffer;
} fd_spi_transfer;

void fd_spi0_io(fd_spi_transfer *tranfers, uint32_t count, void (*callback)(void));

void fd_spi0_wait(void);

// synchronous call that writes the address and reads a 1-byte result
uint8_t fd_spi0_read(uint8_t address);

#endif