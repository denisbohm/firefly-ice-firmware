#include "fd_map.h"

// binary dictionary format:
// - uint16_t number of dictionary entries
// - for each dictionary entry:
//   - uint8_t length of key
//   - uint8_t type of value
//   - uint16_t length of value
//   - uint16_t offset of key, value bytes

void fd_map_get(const char *key, uint8_t type, uint8_t **value, uint16_t *length) {
    *value = 0;
    *length = 0;
}