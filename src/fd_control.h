#ifndef FD_CONTROL_H
#define FD_CONTROL_H

#include "fd_binary.h"
#include "fd_packet.h"

#include <stdint.h>

typedef void (*fd_control_command_t)(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length);

typedef void (*fd_control_get_property_fn_t)(fd_binary_t *binary, fd_packet_output_t *packet_output);
typedef void (*fd_control_set_property_fn_t)(fd_binary_t *binary, fd_packet_output_t *packet_output);

void fd_control_initialize(void);

void fd_control_set_command(uint8_t code, fd_control_command_t command);
void fd_control_add_property(uint32_t identifier, fd_control_get_property_fn_t get, fd_control_get_property_fn_t set);

fd_binary_t *fd_control_send_start(fd_packet_output_t *packet_output, uint8_t type);
bool fd_control_send_complete(fd_packet_output_t *packet_output);

uint32_t fd_control_provision_get_utf8(const char *key, uint8_t **value);
uint32_t fd_control_get_name(uint8_t **name);

void fd_control_process(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length);

void fd_control_notify(uint32_t properties);

typedef void (*fd_control_callback_t)(uint8_t code);

extern fd_control_callback_t fd_control_before_callback;
extern fd_control_callback_t fd_control_after_callback;

#endif
