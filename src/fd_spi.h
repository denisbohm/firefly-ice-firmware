#ifndef FD_SPI_H
#define FD_SPI_H

#include <stdbool.h>
#include <stdint.h>

#define FD_SPI_BUS_0 0
#define FD_SPI_BUS_1 1

#define FD_SPI_BUS_0_SLAVE_W25Q16DW ((FD_SPI_BUS_0 << 16) | 0)
#define FD_SPI_BUS_1_SLAVE_LIS3DH ((FD_SPI_BUS_1 << 16) | 0)
#define FD_SPI_BUS_1_SLAVE_NRF8001 ((FD_SPI_BUS_1 << 16) | 1)

typedef uint32_t fd_spi_bus_t;
typedef uint32_t fd_spi_device_t;

typedef enum {fd_spi_op_read = 0x01, fd_spi_op_write = 0x02, fd_spi_op_read_write = 0x03} fd_spi_op;

typedef struct {
    fd_spi_op op;

    uint8_t *tx_buffer;
    uint32_t tx_size;
    uint32_t tx_length;

    uint8_t *rx_buffer;
    uint32_t rx_size;
    uint32_t rx_length;
} fd_spi_transfer_t;

#define FD_SPI_OPTION_NO_CSN 0x00000001
#define FD_SPI_OPTION_VARLEN 0x00000002

typedef struct {
    uint32_t options;
    fd_spi_transfer_t *transfers;
    uint32_t transfers_count;
    void (*completion_callback)(void);
} fd_spi_io_t;

// initialize all spi ports into sleep mode
void fd_spi_initialize(void);

// powers up and routes spi to pins
void fd_spi_on(fd_spi_bus_t bus);
// powers down and unroutes spi from pins
void fd_spi_off(fd_spi_bus_t bus);

void fd_spi_sleep(fd_spi_bus_t bus);
void fd_spi_wake(fd_spi_bus_t bus);

// start an asynchronous transfer
void fd_spi_io(fd_spi_device_t device, fd_spi_io_t *io);
// wait for an asynchronous transfer to complete
void fd_spi_wait(fd_spi_bus_t bus);

// for use with FD_SPI_OPTION_NO_CSN option
void fd_spi_set_device(fd_spi_device_t device);
void fd_spi_chip_select(fd_spi_device_t device, bool select);

// synchronous convenience functions
void fd_spi_sync_tx1(fd_spi_device_t device, uint8_t tx_byte);
void fd_spi_sync_tx2(fd_spi_device_t device, uint8_t byte0, uint8_t byte1);
void fd_spi_sync_txn(fd_spi_device_t device, uint8_t *tx_bytes, uint32_t tx_length);
uint8_t fd_spi_sync_tx1_rx1(fd_spi_device_t device, uint8_t tx_byte);
void fd_spi_sync_txn_rxn(fd_spi_device_t device, uint8_t* tx_bytes, uint32_t tx_length, uint8_t* rx_bytes, uint32_t rx_length);

#endif