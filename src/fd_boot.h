#ifndef FD_BOOT_H
#define FD_BOOT_H

#include <stdint.h>

typedef struct {
    uint32_t magic;
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t capabilities;
    uint8_t git_commit[20];
} fd_boot_data_t;

#define FD_BOOT_MAGIC 0xb001da1a

#endif