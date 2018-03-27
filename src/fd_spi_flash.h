#ifndef FD_SPI_FLASH_H
#define FD_SPI_FLASH_H

#include "fd_spim.h"

#include <stdint.h>

#define FD_SPI_FLASH_PAGE_PROGRAM_3B 0x02
#define FD_SPI_FLASH_READ_STATUS 0x05
#define FD_SPI_FLASH_WRITE_ENABLE 0x06
#define FD_SPI_FLASH_FAST_READ_3B 0x0b
#define FD_SPI_FLASH_SECTOR_ERASE_3B 0x20
#define FD_SPI_FLASH_CHIP_ERASE 0x60
#define FD_SPI_FLASH_READ_MANUFACTURER_DEVICE_ID 0x90
#define FD_SPI_FLASH_READ_IDENTIFICATION 0x9f
#define FD_SPI_FLASH_POWER_DOWN 0xb9
#define FD_SPI_FLASH_RELEASE_POWER_DOWN 0xab

#define FD_SPI_FLASH_FAST_READ_4B 0x0c
#define FD_SPI_FLASH_PAGE_PROGRAM_4B 0x12
#define FD_SPI_FLASH_SECTOR_ERASE_4B 0x21

#define FD_SPI_FLASH_STATUS_WRITE_ENABLE 0x02
#define FD_SPI_FLASH_STATUS_BUSY 0x01

#define FD_SPI_FLASH_PAGE_SIZE 256
#define FD_SPI_FLASH_PAGES_PER_SECTOR 16

void fd_spi_flash_wake(fd_spim_device_t *device, uint32_t wake_delay_us);
void fd_spi_flash_sleep(fd_spim_device_t *device);

typedef struct {
    uint8_t manufacturer_id;
    uint8_t device_id;
    uint8_t memory_type;
    uint8_t memory_capacity;
} fd_spi_flash_information_t;

void fd_spi_flash_get_information(fd_spim_device_t *device, fd_spi_flash_information_t *information);

void fd_spi_flash_enable_write(fd_spim_device_t *device);
void fd_spi_flash_wait_while_busy(fd_spim_device_t *device);

void fd_spi_flash_chip_erase(fd_spim_device_t *device);
void fd_spi_flash_erase_sector(fd_spim_device_t *device, uint32_t address);

void fd_spi_flash_write_page(fd_spim_device_t *device, uint32_t address, uint8_t *data, uint32_t length);

void fd_spi_flash_read(fd_spim_device_t *device, uint32_t address, uint8_t *data, uint32_t length);

#endif