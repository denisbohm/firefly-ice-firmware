#include "fdi_spi.h"

#include "fdi_delay.h"
#include "fdi_gpio.h"

#include "fd_log.h"

#define FDI_SPI_FAST

void fdi_spi_initialize(void) {
}

#define fdi_spi_delay()

void fdi_spi_enable(uint32_t identifier) {
    fdi_gpio_off(identifier);
}

void fdi_spi_disable(uint32_t identifier) {
    fdi_gpio_on(identifier);
}

#if !defined(FDI_SPI_FAST)
uint8_t fdi_spi_io(uint8_t data) {
    uint8_t in = 0;
    for (int i = 0; i < 8; ++i) {
        fdi_gpio_off(FDI_GPIO_SPI_SCLK);
        if (data & 0x80) {
            fdi_gpio_on(FDI_GPIO_SPI_MOSI);
        } else {
            fdi_gpio_off(FDI_GPIO_SPI_MOSI);
        }
        data = data << 1;
        fdi_spi_delay();

        fdi_gpio_on(FDI_GPIO_SPI_SCLK);
        fdi_spi_delay();
        in = (in << 1) | fdi_gpio_get(FDI_GPIO_SPI_MISO);
    }
    return in;
}
#endif

void fdi_spi_out(uint8_t* txdata, uint32_t txdata_length) {
    fdi_gpio_spi_out(FDI_GPIO_SPI_SCLK, FDI_GPIO_SPI_MOSI, FDI_GPIO_SPI_MISO, txdata, txdata_length);
}

void fdi_spi_txn_then_rxn(uint32_t identifier, uint8_t* txdata, uint32_t txdata_length, uint8_t* rxdata, uint32_t rxdata_length) {
    fdi_spi_enable(identifier);
#if defined(FDI_SPI_FAST)
    fdi_gpio_spi_out(FDI_GPIO_SPI_SCLK, FDI_GPIO_SPI_MOSI, FDI_GPIO_SPI_MISO, txdata, txdata_length);
    fdi_gpio_spi_in(FDI_GPIO_SPI_SCLK, FDI_GPIO_SPI_MOSI, FDI_GPIO_SPI_MISO, rxdata, rxdata_length);
#else
    for (uint32_t i = 0; i < txdata_length; ++i) {
        fdi_spi_io(txdata[i]);
    }
    for (uint32_t i = 0; i < rxdata_length; ++i) {
        rxdata[i] = fdi_spi_io(0);
    }
#endif
    fdi_spi_disable(identifier);
}

uint8_t fdi_spi_tx1_rx1(uint32_t identifier, uint8_t data) {
    uint8_t rx;
    fdi_spi_txn_then_rxn(identifier, &data, 1, &rx, 1);
    return rx;
}

void fdi_spi_tx1(uint32_t identifier, uint8_t data) {
    fdi_spi_txn_then_rxn(identifier, &data, 1, 0, 0);
}