#include "fd_i2cm.h"

bool fd_i2cm_device_txn(const fd_i2cm_device_t *device, const uint8_t *tx_bytes, uint32_t tx_byte_count) {
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = (uint8_t *)tx_bytes,
            .byte_count = tx_byte_count,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = 1,
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

bool fd_i2cm_device_rxn(const fd_i2cm_device_t *device, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_rx,
            .bytes = rx_bytes,
            .byte_count = rx_byte_count,
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = 1,
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

bool fd_i2cm_device_sequence_tx1_rx1(const fd_i2cm_device_t *device, uint8_t tx_byte, uint8_t *rx_byte) {
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

bool fd_i2cm_device_sequence_tx1_tx1(const fd_i2cm_device_t *device, uint8_t tx_byte0, uint8_t tx_byte1) {
    uint8_t tx_bytes[] = {tx_byte0, tx_byte1};
    fd_i2cm_transfer_t transfers[] = {
        {
            .direction = fd_i2cm_direction_tx,
            .bytes = tx_bytes,
            .byte_count = sizeof(tx_bytes),
        },
    };
    fd_i2cm_io_t io = {
        .transfers = transfers,
        .transfer_count = 1,
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