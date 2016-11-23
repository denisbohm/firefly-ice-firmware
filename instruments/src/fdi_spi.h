#ifndef FDI_SPI_H
#define FDI_SPI_H

#include <stdbool.h>
#include <stdint.h>

void fdi_spi_initialize(void);

void fdi_spi_enable(uint32_t identifier);
void fdi_spi_disable(uint32_t identifier);

uint8_t fdi_spi_io(uint8_t data);
void fdi_spi_out(uint8_t* txdata, uint32_t txdata_length);

void fdi_spi_txn_then_rxn(uint32_t identifier, uint8_t* txdata, uint32_t txdata_length, uint8_t* rxdata, uint32_t rxdata_length);
uint8_t fdi_spi_tx1_rx1(uint32_t identifier, uint8_t data);
void fdi_spi_tx1(uint32_t identifier, uint8_t data);

#endif
