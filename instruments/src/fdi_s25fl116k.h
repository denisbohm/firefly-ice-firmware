#ifndef FDI_S25FL116K_H
#define FDI_S25FL116K_H

#include <stdbool.h>
#include <stdint.h>

void fdi_s25fl116k_initialize(void);

extern const uint32_t fdi_s25fl116k_sector_size;

void fdi_s25fl116k_enable_write(void);

void fdi_s25fl116k_chip_erase(void);

void fdi_s25fl116k_erase_sector(uint32_t address);

void fdi_s25fl116k_write_page(uint32_t address, uint8_t *data, uint32_t length);

void fdi_s25fl116k_wait_while_busy(void);

void fdi_s25fl116k_read(uint32_t address, uint8_t *data, uint32_t length);

#endif