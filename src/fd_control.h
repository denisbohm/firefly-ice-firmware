#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#include "fd_binary.h"
#include "fd_detour.h"

#include <stdint.h>

typedef void (*fd_control_command_t)(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

void fd_control_initialize(void);

void fd_control_set_command(uint8_t code, fd_control_command_t command);

fd_binary_t *fd_control_send_start(fd_detour_source_collection_t *detour_source_collection, uint8_t type);
void fd_control_send_complete(fd_detour_source_collection_t *detour_source_collection);

uint32_t fd_control_get_name(uint8_t **name);

void fd_control_process(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

typedef void (*fd_control_callback_t)(uint8_t code);

extern fd_control_callback_t fd_control_before_callback;
extern fd_control_callback_t fd_control_after_callback;

#endif