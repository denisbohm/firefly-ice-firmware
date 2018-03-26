#include "fd_i2cm.h"

#include "fd_nrf5.h"

typedef struct {
    bool enabled;
    nrf_drv_twi_t instance;
} fd_i2cm_peripheral_t;

fd_i2cm_peripheral_t fd_i2cm_peripherals[] = {
    {
        .enabled = false,
        .instance = NRF_DRV_TWI_INSTANCE(0),
    },
    {
        .enabled = false,
        .instance = NRF_DRV_TWI_INSTANCE(1),
    },
};

void fd_i2cm_initialize(
    fd_i2cm_bus_t *buses, uint32_t bus_count,
    fd_i2cm_device_t *devices, uint32_t device_count
) {
    for (uint32_t i = 0; i < bus_count; ++i) {
        fd_i2cm_bus_t *bus = &buses[i];
        fd_i2cm_bus_disable(bus);
    }
}

void fd_i2cm_bus_enable(fd_i2cm_bus_t *bus) {
    fd_i2cm_peripheral_t *peripheral = &fd_i2cm_peripherals[bus->instance];
    const nrf_drv_twi_config_t config = {
        .scl = bus->scl.pin,
        .sda = bus->sda.pin,
        .frequency = (nrf_twi_frequency_t)TWI_DEFAULT_CONFIG_FREQUENCY,
        .interrupt_priority = TWI_DEFAULT_CONFIG_IRQ_PRIORITY,
        .clear_bus_init = 1,
        .hold_bus_uninit = TWI_DEFAULT_CONFIG_HOLD_BUS_UNINIT,
    };
    uint32_t err_code = nrf_drv_twi_init(&peripheral->instance, &config, 0, 0);
    APP_ERROR_CHECK(err_code);
    nrf_drv_twi_enable(&peripheral->instance);
    peripheral->enabled = true;
}

void fd_i2cm_bus_disable(fd_i2cm_bus_t *bus) {
    fd_i2cm_peripheral_t *peripheral = &fd_i2cm_peripherals[bus->instance];
    nrf_drv_twi_disable(&peripheral->instance);
    nrf_drv_twi_uninit(&peripheral->instance);
    peripheral->enabled = false;
}

bool fd_i2cm_bus_is_enabled(fd_i2cm_bus_t *bus) {
    return fd_i2cm_peripherals[bus->instance].enabled;
}

static
void fd_i2cm_check_and_recover(fd_i2cm_bus_t *bus, bool success) {
    if (!success) {
        fd_i2cm_bus_enable(bus);
    }
}

bool fd_i2cm_device_io(fd_i2cm_device_t *device, fd_i2cm_io_t *io) {
    if (io->transfer_count == 0) {
        return true;
    }
    // !!! This only handles the 4 cases implemented by nrf_drv_twi. -denis
    fd_i2cm_peripheral_t *peripheral = &fd_i2cm_peripherals[device->bus->instance];
    nrf_drv_twi_xfer_desc_t desc;
    if (io->transfer_count == 1) {
        fd_i2cm_transfer_t *transfer = &io->transfers[0];
        if (transfer->direction == fd_i2cm_direction_tx) {
            desc = (nrf_drv_twi_xfer_desc_t)NRF_DRV_TWI_XFER_DESC_TX(device->address, transfer->bytes, transfer->byte_count);
        } else {
            desc = (nrf_drv_twi_xfer_desc_t)NRF_DRV_TWI_XFER_DESC_RX(device->address, transfer->bytes, transfer->byte_count);
        }
    } else
    if (io->transfer_count == 2) {
        fd_i2cm_transfer_t *transfer0 = &io->transfers[0];
        if (transfer0->direction == fd_i2cm_direction_rx) {
            return false;
        }
        fd_i2cm_transfer_t *transfer1 = &io->transfers[1];
        if (transfer1->direction == fd_i2cm_direction_tx) {
            desc = (nrf_drv_twi_xfer_desc_t)NRF_DRV_TWI_XFER_DESC_TXTX(device->address, transfer0->bytes, transfer0->byte_count, transfer1->bytes, transfer1->byte_count);
        } else {
            desc = (nrf_drv_twi_xfer_desc_t)NRF_DRV_TWI_XFER_DESC_TXRX(device->address, transfer0->bytes, transfer0->byte_count, transfer1->bytes, transfer1->byte_count);
        }
    } else {
        return false;
    }
    ret_code_t err_code = nrf_drv_twi_xfer(&peripheral->instance, &desc, 0);
    APP_ERROR_CHECK(err_code);
    bool success = err_code == NRF_SUCCESS;
    fd_i2cm_check_and_recover(device->bus, success);
    return success;
}

bool fd_i2cm_bus_wait(fd_i2cm_bus_t *bus) {
}

bool fd_i2cm_device_sequence_tx1_rx1(fd_i2cm_device_t *device, uint8_t tx_byte, uint8_t *rx_byte) {
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = &tx_byte,
            .byte_count = 1,
        },
        {
            .direction = fd_i2cm_direction_rx,
            .bytes = rx_byte,
            .byte_count = 1,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = 2,
        .completion_callback = 0,
    };
    if (!fd_i2cm_device_io(device, &io)) {
        return false;
    }
    if (!fd_i2cm_bus_wait(device->bus)) {
        return false;
    }
    return true;
}

bool fd_i2cm_device_sequence_tx1_tx1(fd_i2cm_device_t *device, uint8_t tx_byte0, uint8_t tx_byte1) {
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = &tx_byte0,
            .byte_count = 1,
        },
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = &tx_byte1,
            .byte_count = 1,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = 2,
        .completion_callback = 0,
    };
    if (!fd_i2cm_device_io(device, &io)) {
        return false;
    }
    if (!fd_i2cm_bus_wait(device->bus)) {
        return false;
    }
    return true;
}