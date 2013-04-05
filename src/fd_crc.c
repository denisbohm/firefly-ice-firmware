#include "fd_crc.h"

// CRC-16-CCIT
uint16_t fd_crc_16(uint16_t seed, uint8_t *data, uint32_t length) {
    uint16_t crc = seed;
    uint8_t *end = &data[length];
    for (; data < end; data++) {
        crc = (crc >> 8) | (crc << 8);
        crc ^= *data;
        crc ^= (crc & 0xff) >> 4;
        crc ^= crc << 12;
        crc ^= (crc & 0xff) << 5;
    }
    return crc;
}