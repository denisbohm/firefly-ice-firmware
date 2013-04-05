#ifndef FD_CRC_H
#define FD_CRC_H

#include <stdint.h>

uint16_t fd_crc_16(uint16_t seed, uint8_t *data, uint32_t length);

#endif