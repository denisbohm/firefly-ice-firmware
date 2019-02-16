#ifndef FD_SPIS_H
#define FD_SPIS_H

#include "fd_gpio.h"
#include "fd_spim.h"

#include <stddef.h>
#include <stdint.h>

typedef void (*fd_spis_callback_t)(void);

typedef struct {
    uint32_t instance;
    fd_gpio_t sclk;
    fd_gpio_t mosi;
    fd_gpio_t miso;
    fd_gpio_t csn;
    fd_gpio_t ready;
    uint32_t frequency;
    fd_spim_mode_t mode;
    fd_spis_callback_t callback;
} fd_spis_device_t;

void fd_spis_initialize(const fd_spis_device_t *devices, uint32_t device_count);

bool fd_spis_device_is_enabled(const fd_spis_device_t *device);

void fd_spis_device_enable(const fd_spis_device_t *device);

void fd_spis_device_disable(const fd_spis_device_t *device);

size_t fd_spis_get_buffer_size(const fd_spis_device_t *device);

size_t fd_spis_device_slave_in(const fd_spis_device_t *device, uint8_t *data, size_t size);

void fd_spis_device_master_out(const fd_spis_device_t *device, const uint8_t *data, size_t length);

void fd_spis_device_ready(const fd_spis_device_t *device);

#endif