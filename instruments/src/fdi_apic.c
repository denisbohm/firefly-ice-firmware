#include "fdi_apic.h"

#include "fdi_detour.h"

#include "fd_log.h"

#include <string.h>

typedef enum {
    apiTypeResetInstruments = 0,
    apiTypeDiscoverInstruments = 1,
    apiTypeEcho = 2,
} fdi_apic_api_type_t;

void fdi_apic_initialize(fdi_apic_t *apic, const fd_i2cm_device_t *device) {
    memset(apic, 0, sizeof(fdi_apic_t));
    apic->device = device;
}

bool fdi_apic_write(
    fdi_apic_t *apic,
    uint32_t identifier,
    uint32_t api,
    const uint8_t *content,
    size_t count
) {
    fd_binary_initialize(&apic->packet, apic->data, sizeof(apic->data));
    fd_binary_put_varuint(&apic->packet, identifier);
    fd_binary_put_varuint(&apic->packet, api);
    fd_binary_put_varuint(&apic->packet, count);
    uint32_t header_count = apic->packet.put_index;

    fd_binary_initialize(&apic->packet, apic->data, sizeof(apic->data));
    fd_binary_put_varuint(&apic->packet, header_count + count);
    fd_binary_put_varuint(&apic->packet, identifier);
    fd_binary_put_varuint(&apic->packet, api);
    fd_binary_put_varuint(&apic->packet, count);
    fd_binary_put_bytes(&apic->packet, content, count);

    uint8_t *data = apic->data;
    uint8_t *subdata = apic->subdata;
    uint32_t sequence_number = 0;
    uint32_t offset = 0;
    uint32_t remaining = apic->packet.put_index;
    while (remaining > 0) {
        uint32_t sublength = remaining >= 63 ? 63 : remaining;
        fd_binary_t subdata;
        fd_binary_initialize(&subdata, apic->subdata, sizeof(apic->subdata));
        fd_binary_put_uint8(&subdata, sequence_number);
        fd_binary_put_bytes(&subdata, &data[offset], sublength);
        fd_i2cm_transfer_t transfers[] = {
            {
                .direction = fd_i2cm_direction_tx,
                .bytes = apic->subdata,
                .byte_count = sizeof(apic->subdata),
            },
        };
        fd_i2cm_io_t io = {
            .transfers = transfers,
            .transfer_count = sizeof(transfers) / sizeof(transfers[0]),
        };
        if (!fd_i2cm_device_io(apic->device, &io)) {
            return false;
        }
        remaining -= sublength;
    }
    return true;
}

bool fdi_apic_rx(fdi_apic_t *apic, fdi_apic_response_t *response) {
    fdi_detour_t detour;
    fdi_detour_initialize(&detour, apic->data, sizeof(apic->data));
    while (detour.state != fdi_detour_state_success) {
        fd_i2cm_transfer_t transfers[] = {
            {
                .direction = fd_i2cm_direction_rx,
                .bytes = apic->subdata,
                .byte_count = sizeof(apic->subdata),
            },
        };
        fd_i2cm_io_t io = {
            .transfers = transfers,
            .transfer_count = sizeof(transfers) / sizeof(transfers[0]),
        };
        if (!fd_i2cm_device_io(apic->device, &io)) {
            return false;
        }
        fdi_detour_event(&detour, apic->subdata, sizeof(apic->subdata));
        if (detour.state == fdi_detour_state_error) {
            return false;
        }
    }
    response->data = apic->data;
    response->count = detour.length;
    return true;
}

bool fdi_apic_read(
    fdi_apic_t *apic,
    uint32_t identifier,
    uint32_t api,
    fdi_apic_response_t *response
) {
    fdi_apic_response_t raw = { .data = 0, .count = 0 };
    while (raw.count == 0) {
        if (!fdi_apic_rx(apic, &raw)) {
            return false;
        }
    }

    fd_binary_initialize(&apic->packet, (uint8_t *)raw.data, raw.count);
    uint32_t return_identifier = (uint32_t)fd_binary_get_varuint(&apic->packet);
    fd_log_assert(return_identifier == identifier);
    uint32_t return_api = (uint32_t)fd_binary_get_varuint(&apic->packet);
    fd_log_assert(return_api == api);
    uint32_t count = (uint32_t)fd_binary_get_varuint(&apic->packet);
    response->data = &apic->data[apic->packet.get_index];
    response->count = count;
    return true;
}

bool fdi_apic_call(
    fdi_apic_t *apic,
    uint32_t identifier,
    uint32_t api,
    const uint8_t *content,
    size_t count,
    fdi_apic_response_t *response
) {
    if (!fdi_apic_write(apic, identifier, api, content, count)) {
        return false;
    }
    if (!fdi_apic_read(apic, identifier, api, response)) {
        return false;
    }
    return true;
}

void fdi_apic_discover_instruments_response(
    fdi_apic_t *apic, const uint8_t *data, size_t size
) {
    apic->instrument_count = 0;
    apic->identifier_min = 0;
    apic->identifier_max = 0;

    fd_binary_t results;
    fd_binary_initialize(&results, (uint8_t *)data, size);
    uint32_t count = fd_binary_get_varuint(&results);
    for (uint32_t i = 0; i < count; ++i) {
        fd_binary_string_t category = fd_binary_get_string(&results);
        uint32_t identifier = (uint32_t)fd_binary_get_varuint(&results);
        fdi_apic_instrument_t *instrument = &apic->instruments[apic->instrument_count++];
        memcpy(instrument->category, category.data, category.length);
        instrument->category[category.length] = '\0';
        instrument->identifier = identifier;
        if (i == 0) {
            apic->identifier_min = identifier;
            apic->identifier_max = identifier;
        } else {
            if (identifier < apic->identifier_min) {
                apic->identifier_min = identifier;
            }
            if (identifier > apic->identifier_max) {
                apic->identifier_max = identifier;
            }
        }
    }
}

bool fdi_apic_reset_instruments(fdi_apic_t *apic) {
    const uint32_t identifier = 0;
    if (!fdi_apic_write(apic, identifier, apiTypeResetInstruments, 0, 0)) {
        return false;
    }
    return true;
}

bool fdi_apic_echo(fdi_apic_t *apic, const uint8_t *data, size_t count) {
    const uint32_t identifier = 0;
    fdi_apic_response_t response;
    if (!fdi_apic_call(apic, identifier, apiTypeEcho, data, count, &response)) {
        return false;
    }

    fd_log_assert((response.count == count) && (memcmp(response.data, data, count) == 0));
    return true;
}

bool fdi_apic_discover_instruments(fdi_apic_t *apic) {
    const uint32_t identifier = 0;
    fdi_apic_response_t response;
    if (!fdi_apic_call(apic, identifier, apiTypeDiscoverInstruments, 0, 0, &response)) {
        return false;
    }

    fdi_apic_discover_instruments_response(apic, response.data, response.count);
    return true;
}

fdi_apic_instrument_t *fdi_apic_get_instrument(fdi_apic_t *apic, uint32_t identifier) {
    for (int i = 0; i < apic->instrument_count; ++i) {
        fdi_apic_instrument_t *instrument = &apic->instruments[i];
        if (instrument->identifier == identifier) {
            return instrument;
        }
    }
    return 0;
}
