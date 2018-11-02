#include "fd_cobs.h"

size_t fd_cobs_encode(const uint8_t *src_data, size_t src_length, uint8_t *dst_data, size_t dst_length) {
    uint8_t *dst = dst_data;
    uint8_t *dst_end = dst_data + dst_length;
    uint8_t *code_ptr = dst++;
    uint8_t code = 1;
    const uint8_t *src = src_data;
    const uint8_t *src_end = src_data + src_length;
    while (src < src_end) {
        if (code != 255) {
            uint8_t byte = *src++;
            if (byte != 0) {
                if (dst >= dst_end) {
                    return 0;
                }
                *dst++ = byte;
                code++;
                continue;
            }
        }
        if (code_ptr >= dst_end) {
            return 0;
        }
        *code_ptr = code;
        code_ptr = dst++;
        code = 1;
    }
    if (code_ptr >= dst_end) {
        return 0;
    }
    *code_ptr = code;
    return dst - dst_data;
}

size_t fd_cobs_decode(const uint8_t *src_data, size_t src_length, uint8_t *dst_data, size_t dst_length) {
    uint8_t *dst = dst_data;
    uint8_t *dst_end = dst_data + dst_length;
    uint8_t code = 255;
    uint8_t copy = 0;
    const uint8_t *src = src_data;
    const uint8_t *src_end = src_data + src_length;
    for (; src < src_end; copy--) {
        if (copy != 0) {
            if ((dst >= dst_end) || (src >= src_end)) {
                return 0;
            }
            *dst++ = *src++;
        } else {
            if (code != 255) {
                if (dst >= dst_end) {
                    return 0;
                }
                *dst++ = 0;
            }
            if (src >= src_end) {
                return 0;
            }
            code = *src++;
            copy = code;
        }
    }
    return dst - dst_data;
}
