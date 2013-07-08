#ifndef FD_MAP_H
#define FD_MAP_H

#include <stdint.h>

#define FD_MAP_TYPE_UTF8 0x01

void fd_map_get(const char *key, uint8_t type, uint8_t **value, uint16_t *length);

#endif