#include "fd_log.h"
#include "fd_processor.h"
#include "fd_w25q16dw.h"

#include <em_gpio.h>

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
#define W25Q16DW_DEVICE_ID 0x14

void fd_spi_power_on(void) {
    GPIO_PinOutSet(US0_PWR_PORT_PIN);
    GPIO_PinOutSet(MEM_CSN_PORT_PIN);
    GPIO_PinOutSet(US0_CLK_PORT_PIN);
    GPIO_PinOutSet(US0_MOSI_PORT_PIN);

    fd_delay_ms(10); // tPUW
}

void fd_spi_power_off(void) {
    GPIO_PinOutClear(US0_PWR_PORT_PIN);
    GPIO_PinOutClear(MEM_CSN_PORT_PIN);
    GPIO_PinOutClear(US0_CLK_PORT_PIN);
    GPIO_PinOutClear(US0_MOSI_PORT_PIN);

    fd_delay_ms(10);
}

void fd_spi_enable(void) {
    GPIO_PinOutClear(MEM_CSN_PORT_PIN);
}

void fd_spi_disable(void) {
    GPIO_PinOutSet(MEM_CSN_PORT_PIN);
    GPIO_PinOutSet(US0_MOSI_PORT_PIN);
}

#define fd_spi_delay()

uint8_t mem_data[256];
uint8_t mem_index = 0;

uint8_t fd_spi_bbio(uint8_t data) {
    uint8_t in = 0;
    for (int i = 0; i < 8; ++i) {
        GPIO_PinOutClear(US0_CLK_PORT_PIN);
        if (data & 0x80) {
            GPIO_PinOutSet(US0_MOSI_PORT_PIN);
        } else {
            GPIO_PinOutClear(US0_MOSI_PORT_PIN);
        }
        data = data << 1;
        fd_spi_delay();

        GPIO_PinOutSet(US0_CLK_PORT_PIN);
        fd_spi_delay();
        in = (in << 1) | GPIO_PinInGet(US0_MISO_PORT_PIN);
    }
    mem_data[mem_index++] = in;
    return in;
}

void fd_spi_txn_rxn(uint8_t* txdata, uint32_t txdata_length, uint8_t* rxdata, uint32_t rxdata_length) {
    fd_spi_enable();
    for (uint32_t i = 0; i < txdata_length; ++i) {
        fd_spi_bbio(txdata[i]);
    }
    for (uint32_t i = 0; i < rxdata_length; ++i) {
        rxdata[i] = fd_spi_bbio(0);
    }
    fd_spi_disable();
}

uint8_t fd_spi_tx1_rx1(uint8_t data) {
    fd_spi_enable();
    fd_spi_bbio(data);
    uint8_t out = fd_spi_bbio(0);
    fd_spi_disable();
    return out;
}

void fd_spi_tx1(uint8_t data) {
    fd_spi_enable();
    fd_spi_bbio(data);
    fd_spi_disable();
}

void fd_w25q16dw_wake(void) {
    uint8_t txdata[] = {RELEASE_POWER_DOWN, 0, 0, 0};
    uint8_t device_id;
    fd_spi_txn_rxn(txdata, sizeof(txdata), &device_id, 1);
    fd_delay_us(30); // tRES2
}

#if 0
void fd_w25q16dw_test(void) {
    fd_w25q16dw_wake();
    uint32_t address = 0;
    fd_w25q16dw_enable_write();
    fd_w25q16dw_erase_sector(address);
    uint8_t write_data[2] = {0x01, 0x02};
    fd_w25q16dw_enable_write();
    fd_w25q16dw_write_page(address, write_data, sizeof(write_data));
    uint8_t read_data[2] = {0x00, 0x00};
    fd_w25q16dw_read(address, read_data, sizeof(read_data));

    if (write_data[0] != read_data[0]) {
        return;
    }
}
#endif

void fd_w25q16dw_initialize(void) {
    mem_index = 0;
    for (uint32_t i = 0; i < 256; ++i) {
        mem_data[i] = 0;
    }

    fd_spi_power_on();

    fd_w25q16dw_wake();

    uint8_t txdata[] = {READ_MANUFACTURER_DEVICE_ID, 0x00, 0x00, 0x00};
    uint8_t rxdata[2];
    fd_spi_txn_rxn(txdata, sizeof(txdata), rxdata, sizeof(rxdata));
    uint8_t manufacturer_id = rxdata[0];
    uint8_t device_id = rxdata[1];
    if (manufacturer_id != WINBOND_MANUFACTURER_ID) {
        fd_log_assert_fail("");
    }
    if (device_id != W25Q16DW_DEVICE_ID) {
        fd_log_assert_fail("");
    }

//    fd_w25q16dw_test();
}

static
void fd_w25q16dw_wait_while_busy(void) {
    uint8_t status;
    do {
        status = fd_spi_tx1_rx1(READ_STATUS);
    } while (status & BUSY);
}

void fd_w25q16dw_sleep(void) {
    fd_w25q16dw_wait_while_busy();

    fd_spi_tx1(POWER_DOWN);
}

void fd_w25q16dw_enable_write(void) {
    fd_w25q16dw_wait_while_busy();

    fd_spi_tx1(WRITE_ENABLE);
}

// erase a 4K-byte sector
void fd_w25q16dw_erase_sector(uint32_t address) {
    uint8_t buffer[] = {SECTOR_ERASE, address >> 16, address >> 8, address};
    fd_spi_txn_rxn(buffer, sizeof(buffer), 0, 0);
}

// write up to 256-bytes to a page
void fd_w25q16dw_write_page(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_wait_while_busy();

    uint8_t tx_bytes[] = {PAGE_PROGRAM, address >> 16, address >> 8, address};
    fd_spi_enable();
    for (uint32_t i = 0; i < sizeof(tx_bytes); ++i) {
        fd_spi_bbio(tx_bytes[i]);
    }
    for (uint32_t i = 0; i < length; ++i) {
        fd_spi_bbio(data[i]);
    }
    fd_spi_disable();
}

void fd_w25q16dw_read(uint32_t address, uint8_t *data, uint32_t length) {
    fd_w25q16dw_wait_while_busy();

    uint8_t tx_bytes[] = {FAST_READ, address >> 16, address >> 8, address, 0};
    fd_spi_txn_rxn(tx_bytes, sizeof(tx_bytes), data, length);
}
