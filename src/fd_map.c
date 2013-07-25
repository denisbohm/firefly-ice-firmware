#include "fd_map.h"

#include <string.h>

// binary dictionary format:
// - uint16_t number of dictionary entries
// - for each dictionary entry:
//   - uint8_t length of key
//   - uint8_t type of value
//   - uint16_t length of value
//   - uint16_t offset of key, value bytes

void fd_map_get(uint8_t *map, const char *key, uint8_t type, uint8_t **value, uint16_t *length) {
    *value = 0;
    *length = 0;

    uint8_t *p = map;
    uint16_t count = *((uint16_t *)p);
    p += 2;
    if (count == 0xffff) { // nothing written to flash yet
        return;
    }
    uint8_t *content = p + 6 * count;
    uint16_t key_length = strlen(key);
    for (int i = 0; i < count; ++i) {
        uint8_t entry_key_length = *p++;
        uint8_t value_type = *p++;
        uint16_t value_length = *((uint16_t *)p);
        p += 2;
        uint16_t key_value_offset = *((uint16_t *)p);
        p += 2;
        if (value_type != type) {
            continue;
        }
        if (entry_key_length != key_length) {
            continue;
        }
        if (memcmp(content + key_value_offset, key, key_length) != 0) {
            continue;
        }
        *value = content + key_value_offset + key_length;
        *length = value_length;
        break;
    }
}