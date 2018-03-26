#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_log.h"
#include "fd_spi.h"
#include "fd_w25q16dw.h"

#define PAGE_PROGRAM 0x02
#define READ_STATUS 0x05
#define WRITE_ENABLE 0x06
#define FAST_READ 0x0b
#define SECTOR_ERASE 0x20
#define READ_MANUFACTURER_DEVICE_ID 0x90
#define POWER_DOWN 0xb9
#define RELEASE_POWER_DOWN 0xab

#define WEL 0x02
#define BUSY 0x01

#define WINBOND_MANUFACTURER_ID 0xef

#include <string.h>

void fd_w25q16dw_wake(void) {
    uint8_t txdata[] = {RELEASE_POWER_DOWN, 0, 0, 0};
    uint8_t device_id;
    fd_spi_sync_txn_rxn(FD_SPI_BUS_0_SLAVE_W25Q16DW, txdata, sizeof(txdata), &device_id, 1);
    fd_hal_processor_delay_us(30); // tRES2
}

void fd_w25q16dw_initialize(void) {
    fd_w25q16dw_wake();

    uint8_t txdata[] = {READ_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00};
    uint8_t rxdata[2];
    fd_spi_sync_txn_rxn(FD_SPI_BUS_0_SLAVE_W25Q16DW, txdata, sizeof(txdata), rxdata, sizeof(rxdata));
    uint8_t manufacturer_id = rxdata[0];
    uint8_t device_id = rxdata[1];
    if (manufacturer_id != WINBOND_MANUFACTURER_ID) {
        fd_log_assert_fail("");
    }
}

void fd_w25q16dw_wait_while_busy(void) {
    uint8_t status;
    do {
        status = fd_spi_sync_tx1_rx1(FD_SPI_BUS_0_SLAVE_W25Q16DW, READ_STATUS);
    } while (status & BUSY);
}

void fd_w25q16dw_sleep(void) {
    fd_w25q16dw_wait_while_busy();

    fd_spi_sync_tx1(FD_SPI_BUS_0_SLAVE_W25Q16DW, POWER_DOWN);
}

void fd_w25q16dw_enable_write(void) {
    fd_w25q16dw_wait_while_busy();

    fd_spi_sync_tx1(FD_SPI_BUS_0_SLAVE_W25Q16DW, WRITE_ENABLE);
}

// erase a 4K-byte sector
void fd_w25q16dw_erase_sector(uint32_t address) {
    uint8_t buffer[] = {SECTOR_ERASE, address >> 16, address >> 8, address};
    fd_spi_sync_txn(FD_SPI_BUS_0_SLAVE_W25Q16DW, buffer, sizeof(buffer));
}

// write up to 256-bytes to a page
void fd_w25q16dw_write_page(uint32_t address, uint8_t *data, uint32_t length) {
    uint8_t save[FD_HAL_RESET_CONTEXT_SIZE] = {0};
    fd_hal_reset_push_watchdog_context("mwp", save);

    fd_w25q16dw_wait_while_busy();

    uint8_t tx_bytes[] = {PAGE_PROGRAM, address >> 16, address >> 8, address};
    fd_spi_transfer_t transfers[] = {
        {
            .op = fd_spi_op_write,

            .tx_buffer = tx_bytes,
            .tx_size = sizeof(tx_bytes),
            .tx_length = sizeof(tx_bytes),

            .rx_buffer = 0,
            .rx_size = 0,
            .rx_length = 0,
        },
        {
            .op = fd_spi_op_write,

            .tx_buffer = data,
            .tx_size = length,
            .tx_length = length,

            .rx_buffer = 0,
            .rx_size = 0,
            .rx_length = 0,
        },
    };
    fd_spi_io_t io = {
        .options = 0,
        .transfers = transfers,
        .transfers_count = sizeof(transfers) / sizeof(fd_spi_transfer_t),
        .completion_callback = 0,
    };
    fd_spi_io(FD_SPI_BUS_0_SLAVE_W25Q16DW, &io);
    fd_spi_wait(FD_SPI_BUS_0);

    fd_hal_reset_pop_watchdog_context(save);
}

void fd_w25q16dw_read(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_wait_while_busy();

    uint8_t tx_bytes[] = {FAST_READ, address >> 16, address >> 8, address, 0};
    fd_spi_sync_txn_rxn(FD_SPI_BUS_0_SLAVE_W25Q16DW, tx_bytes, sizeof(tx_bytes), data, length);
}
