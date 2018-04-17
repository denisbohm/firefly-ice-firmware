#include "fd_map.h"

#include <string.h>

// binary dictionary format:
// - uint16_t number of dictionary entries
// - for each dictionary entry:
//   - uint8_t length of key
//   - uint8_t type of value
//   - uint16_t length of value
//   - uint16_t offset of key, value bytes

bool fd_map_get(uint8_t *map, const char *key, uint8_t type, uint8_t **value, uint16_t *length) {
    *value = 0;
    *length = 0;

    uint8_t *p = map;
    uint16_t count = *((uint16_t *)p);
    p += 2;
    if (count == 0xffff) { // nothing written to flash yet
        return false;
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
        if (
            (value_type != type) ||
            (entry_key_length != key_length) ||
            (memcmp(content + key_value_offset, key, key_length) != 0)
        ) {
            continue;
        }
        *value = content + key_value_offset + key_length;
        *length = value_length;
        return true;
    }
    return false;
}

bool fd_map_put(uint8_t *map, uint8_t *new_map, uint16_t *new_map_length, const char *key, uint8_t type, uint8_t *value, uint16_t length) {
    uint8_t *p = map;
    uint16_t count = *((uint16_t *)p);
    p += 2;
    if (count == 0xffff) { // nothing written to flash yet, don't need to copy over any properties
        count = 0;
    }
    bool replace = false;
    uint32_t key_value_length = 0;
    uint8_t *content = p + 6 * count;
    uint16_t key_length = strlen(key);
    for (int i = 0; i < count; ++i) {
        uint8_t entry_key_length = *p++;
        uint8_t value_type = *p++;
        uint16_t value_length = *((uint16_t *)p);
        p += 2;
        uint16_t key_value_offset = *((uint16_t *)p);
        p += 2;
        if (
            (value_type != type) ||
            (entry_key_length != key_length) ||
            (memcmp(content + key_value_offset, key, key_length) != 0)
        ) {
            key_value_length += entry_key_length + value_length;
            continue;
        }
        replace = true;
        key_value_length += entry_key_length + length;
        break;
    }
    
    uint16_t new_count = count;
    if (!replace) {
        ++new_count;
        key_value_length += key_length + length;
    }
    uint32_t new_map_index_size = 2 + new_count * 6;
    uint32_t new_map_size = new_map_index_size + key_value_length;
    if (new_map_size > *new_map_length) {
        return false;
    }
    
    uint32_t new_key_value_offset = 0;
    uint8_t *new_content = &new_map[new_map_index_size];
    *new_map_length = new_map_size;
    uint8_t *new_p = new_map;
    *((uint16_t *)new_p) = new_count;
    new_p += 2;
    p = map + 2;
    for (int i = 0; i < count; ++i) {
        uint8_t entry_key_length = *p++;
        uint8_t value_type = *p++;
        uint16_t value_length = *((uint16_t *)p);
        p += 2;
        uint16_t key_value_offset = *((uint16_t *)p);
        p += 2;
        if (
            (value_type != type) ||
            (entry_key_length != key_length) ||
            (memcmp(content + key_value_offset, key, key_length) != 0)
        ) {
            // retain
            *new_p++ = entry_key_length;
            *new_p++ = value_type;
            *((uint16_t *)new_p) = value_length;
            new_p += 2;
            *((uint16_t *)new_p) = new_key_value_offset;
            new_p += 2;
            memcpy(&new_content[new_key_value_offset], &content[key_value_offset], entry_key_length + value_length);
            new_key_value_offset += entry_key_length + value_length;
        } else {
            // replace
            *new_p++ = entry_key_length;
            *new_p++ = value_type;
            *((uint16_t *)new_p) = length;
            new_p += 2;
            *((uint16_t *)new_p) = new_key_value_offset;
            new_p += 2;
            memcpy(&new_content[new_key_value_offset], &content[key_value_offset], entry_key_length);
            new_key_value_offset += entry_key_length;
            memcpy(&new_content[new_key_value_offset], value, length);
            new_key_value_offset += length;
        }
    }
    if (!replace) {
        // insert
        *new_p++ = key_length;
        *new_p++ = type;
        *((uint16_t *)new_p) = length;
        new_p += 2;
        *((uint16_t *)new_p) = new_key_value_offset;
        new_p += 2;
        memcpy(&new_content[new_key_value_offset], key, key_length);
        new_key_value_offset += key_length;
        memcpy(&new_content[new_key_value_offset], value, length);
        new_key_value_offset += length;
    }
    return true;
}
