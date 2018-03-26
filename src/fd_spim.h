#ifndef FD_SPIM_H
#define FD_SPIM_H

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t *tx_bytes;
    uint32_t tx_byte_count;

    uint8_t *rx_bytes;
    uint32_t rx_byte_count;
} fd_spim_transfer_t;

typedef struct {
    fd_spim_transfer_t *transfers;
    uint32_t transfer_count;
    void (*completion_callback)(void);
} fd_spim_io_t;

typedef enum {
    fd_spim_mode_0,
    fd_spim_mode_1,
    fd_spim_mode_2,
    fd_spim_mode_3,
} fd_spim_mode_t;

typedef struct {
    uint32_t instance;
    fd_gpio_t sclk;
    fd_gpio_t mosi;
    fd_gpio_t miso;
    uint32_t frequency;
    fd_spim_mode_t mode;
} fd_spim_bus_t;

typedef struct {
    fd_spim_bus_t *bus;
    fd_gpio_t csn;
} fd_spim_device_t;

void fd_spim_initialize(
    fd_spim_bus_t *buses, uint32_t bus_count,
    fd_spim_device_t *devices, uint32_t device_count
);

void fd_spim_bus_enable(fd_spim_bus_t *bus);
void fd_spim_bus_disable(fd_spim_bus_t *bus);
bool fd_spim_bus_is_enabled(fd_spim_bus_t *bus);

void fd_spim_device_select(fd_spim_device_t *device);
void fd_spim_device_deselect(fd_spim_device_t *device);
bool fd_spim_device_is_selected(fd_spim_device_t *device);

// start asynchronous I/O
void fd_spim_bus_io(fd_spim_bus_t *bus, fd_spim_io_t *io);
// wait for asynchronous I/O to complete
void fd_spim_bus_wait(fd_spim_bus_t *bus);

// asynchronous convenience functions
void fd_spim_bus_sequence_txn_rxn(fd_spim_bus_t *bus, uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count);
void fd_spim_bus_tx1(fd_spim_bus_t *bus, uint8_t tx_byte);
void fd_spim_bus_rxn(fd_spim_bus_t *bus, uint8_t *rx_bytes, uint32_t rx_byte_count);
void fd_spim_bus_txn(fd_spim_bus_t *bus, uint8_t *tx_bytes, uint32_t tx_byte_count);

// synchronous convenience functions
void fd_spim_device_io(fd_spim_device_t *device, fd_spim_io_t *io);
void fd_spim_device_txn_rxn(fd_spim_device_t *device, uint8_t* tx_bytes, uint32_t tx_byte_count, uint8_t* rx_bytes, uint32_t rx_byte_count);
void fd_spim_device_sequence_txn_rxn(fd_spim_device_t *device, uint8_t* tx_bytes, uint32_t tx_byte_count, uint8_t* rx_bytes, uint32_t rx_byte_count);
uint8_t fd_spim_device_sequence_tx1_rx1(fd_spim_device_t *device, uint8_t tx_byte);
uint8_t fd_spim_device_tx1_rx1(fd_spim_device_t *device, uint8_t tx_byte);
void fd_spim_device_tx1(fd_spim_device_t *device, uint8_t tx_byte);
void fd_spim_device_tx2(fd_spim_device_t *device, uint8_t byte0, uint8_t byte1);
void fd_spim_device_txn(fd_spim_device_t *device, uint8_t *tx_bytes, uint32_t tx_byte_count);

#endif