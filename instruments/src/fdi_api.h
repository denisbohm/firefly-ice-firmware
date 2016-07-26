#ifndef FDI_API_H
#define FDI_API_H

#include "fd_binary.h"

#include <stdbool.h>
#include <stdint.h>

void fdi_api_initialize(void);
void fdi_api_initialize_usb(void);

typedef void (*fdi_api_function_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);
void fdi_api_register(uint64_t identifier, uint64_t type, fdi_api_function_t function);

void fdi_api_process(void);

bool fdi_api_send(uint64_t identifier, uint64_t type, uint8_t *data, uint32_t length);

typedef bool (*fdi_api_can_transmit_handler_t)(void);
typedef void (*fdi_api_transmit_handler_t)(uint8_t *data, uint32_t length);
typedef void (*fdi_api_dispatch_handler_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);

#endif
