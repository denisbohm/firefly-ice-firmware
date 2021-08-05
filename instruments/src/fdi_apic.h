#ifndef FDI_APIC_H
#define FDI_APIC_H

#include "fd_binary.h"
#include "fd_i2cm.h"

#include <stddef.h>

typedef struct {
    char category[32];
    uint32_t identifier;
} fdi_apic_instrument_t;

typedef struct {
    const uint8_t *data;
    size_t count;
} fdi_apic_response_t;

typedef struct {
    const fd_i2cm_device_t *device;

    fd_binary_t packet;
    uint8_t subdata[64];

    uint8_t data[512];
    uint32_t data_count;

    uint32_t instrument_count;
    fdi_apic_instrument_t instruments[32];
    uint32_t identifier_min;
    uint32_t identifier_max;
} fdi_apic_t;

void fdi_apic_initialize(fdi_apic_t *api, const fd_i2cm_device_t *device);

bool fdi_apic_write(
    fdi_apic_t *apic,
    uint32_t identifier,
    uint32_t api,
    const uint8_t *content,
    size_t count
);

bool fdi_apic_call(
    fdi_apic_t *apic,
    uint32_t identifier,
    uint32_t api,
    const uint8_t *content,
    size_t count,
    fdi_apic_response_t *response
);

bool fdi_apic_reset_instruments(fdi_apic_t *apic);
bool fdi_apic_discover_instruments(fdi_apic_t *apic);
bool fdi_apic_echo(fdi_apic_t *apic, const uint8_t *data, size_t count);

fdi_apic_instrument_t *fdi_apic_get_instrument(fdi_apic_t *apic, uint32_t identifier);

bool fdi_apic_rx(fdi_apic_t *apic, fdi_apic_response_t *response);

void fdi_apic_discover_instruments_response(
    fdi_apic_t *apic, const uint8_t *data, size_t size
);

#endif