#ifndef FD_VERSION_H
#define FD_VERSION_H

#include "fd_sha.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_VERSION_MAGIC 0xb001da1a

#define FD_VERSION_METADATA_FLAG_ENCRYPTED 0x00000001

#define FD_VERSION_COMMIT_SIZE 20
#define FD_VERSION_CRYPT_IV_SIZE 16

typedef struct __attribute__ ((__packed__)) {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t capabilities;
    uint8_t commit[FD_VERSION_COMMIT_SIZE];
} fd_version_revision_t;

typedef struct __attribute__ ((__packed__)) {
    uint32_t magic;
    fd_version_revision_t revision;
} fd_version_revision_stored_t;

typedef struct __attribute__ ((__packed__)) {
    uint32_t flags;
    uint32_t length;
    uint8_t hash[FD_SHA_HASH_SIZE];
    uint8_t crypt_hash[FD_SHA_HASH_SIZE];
    uint8_t crypt_iv[FD_VERSION_CRYPT_IV_SIZE];
} fd_version_binary_t;

typedef struct __attribute__ ((__packed__)) {
    fd_version_binary_t binary;
    fd_version_revision_t revision;
} fd_version_metadata_t;

typedef struct __attribute__ ((__packed__)) {
    uint32_t magic;
    fd_version_metadata_t metadata;
} fd_version_metadata_stored_t;

typedef struct __attribute__ ((__packed__)) {
    uint16_t major;
    uint16_t minor;
} fd_version_hardware_t;

#endif
