#ifndef FD_AT24C512C_H
#define FD_AT24C512C_H

#include <stdint.h>

#define FD_AT24C512C_PAGES 512
#define FD_AT24C512C_PAGE_SIZE 128

void fd_at24c512c_initialize(void);
void fd_at24c512c_test(void);

void fd_at24c512c_erase_page(uint32_t address);
void fd_at24c512c_write_page(uint32_t address, uint8_t *data, uint32_t length);
void fd_at24c512c_read_page(uint32_t address, uint8_t *data, uint32_t length);

#endif