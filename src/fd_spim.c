#include "fd_spim.h"

void fd_spim_bus_sequence_txn_rxn(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    fd_spim_transfer_t transfers[] = {
        {
            .tx_bytes = tx_bytes,
            .tx_byte_count = tx_byte_count,
            .rx_bytes = 0,
            .rx_byte_count = 0,
        },
        {
            .tx_bytes = 0,
            .tx_byte_count = 0,
            .rx_bytes = rx_bytes,
            .rx_byte_count = rx_byte_count,
        },
    };
    fd_spim_io_t io = {
        .transfers = transfers,
        .transfer_count = 2,
        .completion_callback = 0,
    };
    fd_spim_bus_io(bus, &io);
}

void fd_spim_bus_tx1(const fd_spim_bus_t *bus, uint8_t tx_byte) {
    fd_spim_transfer_t transfer = {
        .tx_bytes = &tx_byte,
        .tx_byte_count = 1,
        .rx_bytes = 0,
        .rx_byte_count = 0,
    };
    fd_spim_io_t io = {
        .transfers = &transfer,
        .transfer_count = 1,
        .completion_callback = 0,
    };
    fd_spim_bus_io(bus, &io);
}

void fd_spim_bus_rxn(const fd_spim_bus_t *bus, uint8_t *rx_bytes, uint32_t rx_byte_count) {
    fd_spim_transfer_t transfer = {
        .tx_bytes = 0,
        .tx_byte_count = 0,
        .rx_bytes = rx_bytes,
        .rx_byte_count = rx_byte_count,
    };
    fd_spim_io_t io = {
        .transfers = &transfer,
        .transfer_count = 1,
        .completion_callback = 0,
    };
    fd_spim_bus_io(bus, &io);
}

void fd_spim_bus_txn(const fd_spim_bus_t *bus, const uint8_t *tx_bytes, uint32_t tx_byte_count) {
    fd_spim_transfer_t transfer = {
        .tx_bytes = tx_bytes,
        .tx_byte_count = tx_byte_count,
        .rx_bytes = 0,
        .rx_byte_count = 0,
    };
    fd_spim_io_t io = {
        .transfers = &transfer,
        .transfer_count = 1,
        .completion_callback = 0,
    };
    fd_spim_bus_io(bus, &io);
}

void fd_spim_device_io(const fd_spim_device_t *device, const fd_spim_io_t *io) {
    fd_spim_device_select(device);
    fd_spim_bus_io(device->bus, io);
    fd_spim_bus_wait(device->bus);
    fd_spim_device_deselect(device);
}

void fd_spim_device_txn_rxn(const fd_spim_device_t *device, const uint8_t* tx_bytes, uint32_t tx_byte_count, uint8_t* rx_bytes, uint32_t rx_byte_count) {
    fd_spim_transfer_t transfer = {
        .tx_bytes = tx_bytes,
        .tx_byte_count = tx_byte_count,
        .rx_bytes = rx_bytes,
        .rx_byte_count = rx_byte_count,
    };
    fd_spim_io_t io = {
        .transfers = &transfer,
        .transfer_count = 1,
        .completion_callback = 0,
    };
    fd_spim_device_io(device, &io);
}

void fd_spim_device_sequence_txn_rxn(const fd_spim_device_t *device, const uint8_t* tx_bytes, uint32_t tx_byte_count, uint8_t* rx_bytes, uint32_t rx_byte_count) {
    fd_spim_transfer_t transfers[] = {
        {
            .tx_bytes = tx_bytes,
            .tx_byte_count = tx_byte_count,
            .rx_bytes = 0,
            .rx_byte_count = 0,
        },
        {
            .tx_bytes = 0,
            .tx_byte_count = 0,
            .rx_bytes = rx_bytes,
            .rx_byte_count = rx_byte_count,
        },
    };
    fd_spim_io_t io = {
        .transfers = transfers,
        .transfer_count = 2,
        .completion_callback = 0,
    };
    fd_spim_device_io(device, &io);
}

uint8_t fd_spim_device_sequence_tx1_rx1(const fd_spim_device_t *device, uint8_t tx_byte) {
    uint8_t rx_byte;
    fd_spim_device_sequence_txn_rxn(device, &tx_byte, 1, &rx_byte, 1);
    return rx_byte;
}

uint8_t fd_spim_device_tx1_rx1(const fd_spim_device_t *device, uint8_t tx_byte) {
    uint8_t rx_byte;
    fd_spim_device_txn_rxn(device, &tx_byte, 1, &rx_byte, 1);
    return rx_byte;
}

void fd_spim_device_tx1(const fd_spim_device_t *device, uint8_t tx_byte) {
    fd_spim_device_txn_rxn(device, &tx_byte, 1, 0, 0);
}

void fd_spim_device_tx2(const fd_spim_device_t *device, uint8_t byte0, uint8_t byte1) {
    uint8_t tx_bytes[] = {byte0, byte1};
    fd_spim_device_txn_rxn(device, tx_bytes, sizeof(tx_bytes), 0, 0);
}

void fd_spim_device_txn(const fd_spim_device_t *device, const uint8_t *tx_bytes, uint32_t tx_byte_count) {
    fd_spim_device_txn_rxn(device, tx_bytes, tx_byte_count, 0, 0);
}
