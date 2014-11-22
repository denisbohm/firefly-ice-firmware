#ifndef FD_VERSION_H
#define FD_VERSION_H

#include "fd_sha.h"

#include <stdint.h>

#define FD_VERSION_METADATA_MAGIC 0xb001da1a

#define FD_VERSION_COMMIT_SIZE 20

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t capabilities;
    uint8_t commit[FD_VERSION_COMMIT_SIZE];
} fd_version_t;

typedef struct {
    uint32_t length;
    uint8_t hash[FD_SHA_HASH_SIZE];
} fd_version_binary_t;

typedef struct {
    uint32_t magic;
    fd_version_t version;
    fd_version_binary_t binary;
} fd_version_metadata_t;

typedef struct {
    uint16_t major;
    uint16_t minor;
} fd_version_hardware_t;

#endif
