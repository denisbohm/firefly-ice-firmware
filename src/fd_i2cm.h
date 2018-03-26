#ifndef FD_I2CM_H
#define FD_I2CM_H

#include "fd_gpio.h"

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    fd_i2cm_direction_rx,
    fd_i2cm_direction_tx,
} fd_i2cm_direction_t;

typedef struct {
    fd_i2cm_direction_t direction;
    uint8_t *bytes;
    uint32_t byte_count;
} fd_i2cm_transfer_t;

typedef struct {
    fd_i2cm_transfer_t *transfers;
    uint32_t transfer_count;
    void (*completion_callback)(void);
} fd_i2cm_io_t;

typedef struct {
    uint32_t instance;
    fd_gpio_t scl;
    fd_gpio_t sda;
    uint32_t frequency;
} fd_i2cm_bus_t;

typedef struct {
    fd_i2cm_bus_t *bus;
    uint32_t address;
} fd_i2cm_device_t;

void fd_i2cm_initialize(
    fd_i2cm_bus_t *buses, uint32_t bus_count,
    fd_i2cm_device_t *devices, uint32_t device_count
);

void fd_i2cm_bus_enable(fd_i2cm_bus_t *bus);
void fd_i2cm_bus_disable(fd_i2cm_bus_t *bus);
bool fd_i2cm_bus_is_enabled(fd_i2cm_bus_t *bus);

// start asynchronous I/O
bool fd_i2cm_device_io(fd_i2cm_device_t *device, fd_i2cm_io_t *io);
// wait for asynchronous I/O to complete
bool fd_i2cm_bus_wait(fd_i2cm_bus_t *bus);

// synchronous convenience functions
bool fd_i2cm_device_sequence_tx1_rx1(fd_i2cm_device_t *device, uint8_t tx_byte, uint8_t *rx_byte);
bool fd_i2cm_device_sequence_tx1_tx1(fd_i2cm_device_t *device, uint8_t tx_byte0, uint8_t tx_byte1);

#endif