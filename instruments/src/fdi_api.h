#ifndef FDI_API_H
#define FDI_API_H

#include "fdi_apic.h"

#include "fd_binary.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef bool (*fdi_api_can_transmit_handler_t)(void);
typedef void (*fdi_api_transmit_handler_t)(uint8_t *data, uint32_t length);
typedef void (*fdi_api_dispatch_handler_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);

typedef struct {
    bool (*can_transmit)(void);
    void (*transmit)(uint8_t *data, size_t length);
    uint32_t apic_count;
    fdi_apic_t apics[2];
} fdi_api_configuration_t;

extern fdi_api_configuration_t fdi_api_configuration;

void fdi_api_initialize(fdi_api_configuration_t configuration);
void fdi_api_rx_callback(uint8_t *data, uint32_t length);
void fdi_api_tx_callback(void);

typedef void (*fdi_api_function_t)(uint64_t identifier, uint64_t type, fd_binary_t *binary);
void fdi_api_register(uint64_t identifier, uint64_t type, fdi_api_function_t function);

void fdi_api_process(void);

bool fdi_api_send(uint64_t identifier, uint64_t type, uint8_t *data, uint32_t length);

#endif
