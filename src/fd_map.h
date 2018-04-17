#ifndef FD_MAP_H
#define FD_MAP_H

#include <stdbool.h>
#include <stdint.h>

#define FD_MAP_TYPE_UTF8 0x01

// Returns true if key of the proper type is found in the map.
bool fd_map_get(uint8_t *map, const char *key, uint8_t type, uint8_t **value, uint16_t *length);

// Creates a new map with the given key replaced or added as appropriate.
// Pass in the size of the new_map buffer to new_map_length.
// Returns false if there is not enough space available in the new map.
// Return true if there is enough space and sets new_map_length to the actual new map length.
bool fd_map_put(uint8_t *map, uint8_t *new_map, uint16_t *new_map_length, const char *key, uint8_t type, uint8_t *value, uint16_t length);

#endif
