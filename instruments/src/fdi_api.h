#ifndef FDI_API_H
#define FDI_API_H

#include "fd_binary.h"

#include <stdint.h>

void fdi_api_initialize(void);

void fdi_api_initialize_usb(void);

typedef void (*fdi_api_function_t)(fd_binary_t *binary);

void fdi_api_register(uint32_t type, fdi_api_function_t function);

void fdi_api_process(void);

void fdi_api_event(uint8_t *data, uint32_t length);

void fdi_api_send(uint32_t type, uint8_t *data, uint32_t length);

#endif
