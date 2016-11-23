#include "fdi_s25fl116k.h"

#include "fdi_delay.h"
#include "fdi_gpio.h"
#include "fdi_spi.h"

#include "fd_log.h"

#define PAGE_PROGRAM 0x02
#define READ_STATUS 0x05
#define WRITE_ENABLE 0x06
#define FAST_READ 0x0b
#define SECTOR_ERASE 0x20
#define READ_MANUFACTURER_DEVICE_ID 0x90
#define POWER_DOWN 0xb9
#define RELEASE_POWER_DOWN 0xab
#define CHIP_ERASE 0xc7

#define WEL 0x02
#define BUSY 0x01

#define S25FL116K_MANUFACTURER_ID 0x01
#define S25FL116K_DEVICE_ID 0x14

const uint32_t fdi_s25fl116k_sector_size = 4096;

void fdi_s25fl116k_wake(void) {
    uint8_t txdata[] = {RELEASE_POWER_DOWN, 0, 0, 0};
    uint8_t device_id;
    fdi_spi_txn_then_rxn(FDI_GPIO_S25FL116K_CSN, txdata, sizeof(txdata), &device_id, 1);
    fdi_delay_us(30); // tRES2
}

//#define S25FL116K_TEST
#ifdef S25FL116K_TEST

void fdi_s25fl116k_enable_write(void);

void fdi_s25fl116k_test(void) {
    fdi_s25fl116k_wake();
    uint32_t address = 0;
    fdi_s25fl116k_enable_write();
    fdi_s25fl116k_erase_sector(address);
    uint8_t write_data[2] = {0x01, 0x02};
    fdi_s25fl116k_enable_write();
    fdi_s25fl116k_write_page(address, write_data, sizeof(write_data));
    uint8_t read_data[2] = {0x00, 0x00};
    fdi_s25fl116k_read(address, read_data, sizeof(read_data));

    if (write_data[0] != read_data[0]) {
        return;
    }
}
#endif

void fdi_s25fl116k_initialize(void) {
    fdi_s25fl116k_wake();

    uint8_t txdata[] = {READ_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00};
    uint8_t rxdata[2];
    fdi_spi_txn_then_rxn(FDI_GPIO_S25FL116K_CSN, txdata, sizeof(txdata), rxdata, sizeof(rxdata));
    uint8_t manufacturer_id = rxdata[0];
    uint8_t device_id = rxdata[1];
    if (manufacturer_id != S25FL116K_MANUFACTURER_ID) {
        fd_log_assert_fail("");
    }
    if (device_id != S25FL116K_DEVICE_ID) {
        fd_log_assert_fail("");
    }

#ifdef S25FL116K_TEST
    fdi_s25fl116k_test();
#endif
}

void fdi_s25fl116k_wait_while_busy(void) {
    uint8_t status;
    do {
        status = fdi_spi_tx1_rx1(FDI_GPIO_S25FL116K_CSN, READ_STATUS);
    } while (status & BUSY);
}

void fdi_s25fl116k_sleep(void) {
    fdi_s25fl116k_wait_while_busy();

    fdi_spi_tx1(FDI_GPIO_S25FL116K_CSN, POWER_DOWN);
}

void fdi_s25fl116k_enable_write(void) {
    fdi_s25fl116k_wait_while_busy();

    fdi_spi_tx1(FDI_GPIO_S25FL116K_CSN, WRITE_ENABLE);
}

// erase a 4K-byte sector
void fdi_s25fl116k_erase_sector(uint32_t address) {
    fdi_s25fl116k_wait_while_busy();

    uint8_t buffer[] = {SECTOR_ERASE, address >> 16, address >> 8, address};
    fdi_spi_txn_then_rxn(FDI_GPIO_S25FL116K_CSN, buffer, sizeof(buffer), 0, 0);
}

// write up to 256-bytes to a page
void fdi_s25fl116k_write_page(uint32_t address, uint8_t *data, uint32_t length) {
    fdi_s25fl116k_wait_while_busy();

    uint8_t tx_bytes[] = {PAGE_PROGRAM, address >> 16, address >> 8, address};
    fdi_spi_enable(FDI_GPIO_S25FL116K_CSN);
    fdi_spi_out(tx_bytes, sizeof(tx_bytes));
    fdi_spi_out(data, length);
    fdi_spi_disable(FDI_GPIO_S25FL116K_CSN);
}

void fdi_s25fl116k_read(uint32_t address, uint8_t *data, uint32_t length) {
    fdi_s25fl116k_wait_while_busy();

    uint8_t tx_bytes[] = {FAST_READ, address >> 16, address >> 8, address, 0};
    fdi_spi_txn_then_rxn(FDI_GPIO_S25FL116K_CSN, tx_bytes, sizeof(tx_bytes), data, length);
}

void fdi_s25fl116k_chip_erase(void) {
    fdi_s25fl116k_wait_while_busy();

    fdi_spi_tx1(FDI_GPIO_S25FL116K_CSN, CHIP_ERASE);
}
