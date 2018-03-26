#include "fd_spi_flash.h"

#include "fd_delay.h"

#include <string.h>

void fd_spi_flash_wake(fd_spim_device_t *device, uint32_t wake_delay_us) {
    uint8_t tx_bytes[] = {FD_SPI_FLASH_RELEASE_POWER_DOWN, 0, 0, 0};
    uint8_t device_id;
    fd_spim_device_select(device);
    fd_spim_bus_sequence_txn_rxn(device->bus, tx_bytes, sizeof(tx_bytes), &device_id, 1);
    fd_spim_bus_wait(device->bus);
    fd_delay_us(wake_delay_us);
    fd_spim_device_deselect(device);
}

void fd_spi_flash_sleep(fd_spim_device_t *device) {
    fd_spi_flash_wait_while_busy(device);

    fd_spim_device_tx1(device, FD_SPI_FLASH_POWER_DOWN);
}

void fd_spi_flash_get_information(fd_spim_device_t *device, fd_spi_flash_information_t *information) {
    {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_READ_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00};
        uint8_t rx_bytes[2];
        fd_spim_device_sequence_txn_rxn(device, tx_bytes, sizeof(tx_bytes), rx_bytes, sizeof(rx_bytes));
        information->manufacturer_id = rx_bytes[0];
        information->device_id = rx_bytes[1];
    }
    {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_READ_IDENTIFICATION};
        uint8_t rx_bytes[3];
        fd_spim_device_sequence_txn_rxn(device, tx_bytes, sizeof(tx_bytes), rx_bytes, sizeof(rx_bytes));
        information->memory_type = rx_bytes[1];
        information->memory_capacity = rx_bytes[2];
    }
}

void fd_spi_flash_wait_while_busy(fd_spim_device_t *device) {
    uint8_t status;
    do {
        status = fd_spim_device_sequence_tx1_rx1(device, FD_SPI_FLASH_READ_STATUS);
    } while (status & FD_SPI_FLASH_STATUS_BUSY);
}

void fd_spi_flash_enable_write(fd_spim_device_t *device) {
    fd_spi_flash_wait_while_busy(device);

    fd_spim_device_tx1(device, FD_SPI_FLASH_WRITE_ENABLE);
}

void fd_spi_flash_erase_sector(fd_spim_device_t *device, uint32_t address) {
    if ((address & 0xff000000) != 0) {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_SECTOR_ERASE_4B, address >> 24, address >> 16, address >> 8, address};
        fd_spim_device_txn(device, tx_bytes, sizeof(tx_bytes));
    } else {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_SECTOR_ERASE_3B, address >> 16, address >> 8, address};
        fd_spim_device_txn(device, tx_bytes, sizeof(tx_bytes));
    }
}

void fd_spi_flash_write_page(fd_spim_device_t *device, uint32_t address, uint8_t *data, uint32_t length) {
    fd_spi_flash_wait_while_busy(device);

    uint8_t tx_bytes[5];
    uint32_t tx_byte_count;
    if ((address & 0xff000000) != 0) {
        tx_bytes[0] = FD_SPI_FLASH_PAGE_PROGRAM_4B;
        tx_bytes[1] = address >> 24;
        tx_bytes[2] = address >> 16;
        tx_bytes[3] = address >> 8;
        tx_bytes[4] = address;
        tx_byte_count = 5;
    } else {
        tx_bytes[0] = FD_SPI_FLASH_PAGE_PROGRAM_3B;
        tx_bytes[1] = address >> 16;
        tx_bytes[2] = address >> 8;
        tx_bytes[3] = address;
        tx_byte_count = 4;
    }
    fd_spim_transfer_t transfers[] = {
        {
            .tx_bytes = tx_bytes,
            .tx_byte_count = tx_byte_count,

            .rx_bytes = 0,
            .rx_byte_count = 0,
        },
        {
            .tx_bytes = data,
            .tx_byte_count = length,

            .rx_bytes = 0,
            .rx_byte_count = 0,
        },
    };
    fd_spim_io_t io = {
        .transfers = transfers,
        .transfer_count = sizeof(transfers) / sizeof(fd_spim_transfer_t),
        .completion_callback = 0,
    };
    fd_spim_device_io(device, &io);
}

void fd_spi_flash_read(fd_spim_device_t *device, uint32_t address, uint8_t *data, uint32_t length) {
    fd_spi_flash_wait_while_busy(device);

    if ((address & 0xff000000) != 0) {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_FAST_READ_4B, address >> 24, address >> 16, address >> 8, address, 0};
        fd_spim_device_sequence_txn_rxn(device, tx_bytes, sizeof(tx_bytes), data, length);
    } else {
        uint8_t tx_bytes[] = {FD_SPI_FLASH_FAST_READ_3B, address >> 16, address >> 8, address, 0};
        fd_spim_device_sequence_txn_rxn(device, tx_bytes, sizeof(tx_bytes), data, length);
    }
}
