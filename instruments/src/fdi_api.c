#include "fdi_api.h"

#include "fdi_interrupt.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <string.h>

#define fdi_api_send_limit 64

typedef struct {
    uint32_t ordinal;
    uint32_t length;
    uint32_t offset;
} fdi_api_merge_t;

typedef struct {
    uint64_t identifier;
    uint64_t type;
    fdi_api_function_t function;
} fdi_api_entry_t;

#define fdi_api_entry_size 512

typedef struct {
    uint64_t identifier;
    uint64_t type;
} fdi_api_event_t;

#define fdi_api_tx_size 5120
#define fdi_api_rx_size 5120

typedef struct {
    fdi_api_configuration_t configuration;

    fdi_api_entry_t entrys[fdi_api_entry_size];
    uint32_t entry_count;

    uint32_t tx_index;
    uint32_t tx_length;
    uint8_t tx_buffer[fdi_api_tx_size];

    fdi_api_merge_t merge;
    uint32_t rx_index;
    uint32_t rx_length;
    uint8_t rx_buffer[fdi_api_rx_size];

    fdi_api_event_t event_history[256];
    uint32_t event_history_index;
} fdi_api_t;

fdi_api_t fdi_api;

void fdi_api_register(uint64_t identifier, uint64_t type, fdi_api_function_t function) {
    fd_log_assert(fdi_api.entry_count < fdi_api_entry_size);
    fdi_api_entry_t *entry = &fdi_api.entrys[fdi_api.entry_count];
    entry->identifier = identifier;
    entry->type = type;
    entry->function = function;
    fdi_api.entry_count += 1;
}

fdi_api_function_t fdi_api_lookup(uint64_t identifier, uint64_t type) {
    for (uint32_t i = 0; i < fdi_api.entry_count; ++i) {
        fdi_api_entry_t *entry = &fdi_api.entrys[i];
        if ((entry->identifier == identifier) && (entry->type == type)) {
            return entry->function;
        }
    }
    return 0;
}

// remove data from tx buffer
bool fdi_api_process_tx(void) {
    if (fdi_api.tx_length == 0) {
        return false;
    }

    uint8_t length = fdi_api.tx_buffer[fdi_api.tx_index];
    fd_log_assert((fdi_api.tx_index + length) <= fdi_api.tx_length);
    if (!fdi_api.configuration.transmit(&fdi_api.tx_buffer[fdi_api.tx_index + 1], length)) {
        return false;
    }

    fdi_api.tx_index += 1 + length;
    if (fdi_api.tx_index == fdi_api.tx_length) {
        fdi_api.tx_index = 0;
        fdi_api.tx_length = 0;
    }
    return true;
}

void fdi_api_dispatch(uint64_t identifier, uint64_t type, fd_binary_t *binary) {
    fdi_api_event_t *event = &fdi_api.event_history[fdi_api.event_history_index++ & 0xff];
    event->identifier = identifier;
    event->type = type;
    
    fdi_api_function_t function = fdi_api_lookup(identifier, type);
    if (function) {
        function(identifier, type, binary);
        fd_log_assert(binary->flags == 0);
        return;
    }

    for (uint32_t i = 0; i < fdi_api.configuration.apic_count; ++i) {
        fdi_apic_t *apic = &fdi_api.configuration.apics[i];
        if ((apic->identifier_min <= identifier) && (identifier <= apic->identifier_max)) {
            uint8_t *content = &binary->buffer[binary->get_index];
            size_t count = binary->size - binary->get_index;
            fdi_apic_write(apic, identifier, type, content, count);
            return;
        }
    }

    fd_log_assert_fail("unknown api");
}

// remove data from rx buffer
bool fdi_api_process_rx(void) {
    uint32_t primask = fdi_interrupt_disable();
    uint32_t index = fdi_api.rx_index;
    uint32_t length = fdi_api.rx_length;
    fd_binary_t binary;
    fd_binary_initialize(&binary, &fdi_api.rx_buffer[index], length - index);
    fdi_interrupt_restore(primask);

    if (index >= length) {
        return false;
    }

    do {
        uint32_t function_length = (uint32_t)fd_binary_get_uint16(&binary);
        uint64_t identifier = fd_binary_get_varuint(&binary);
        uint64_t type = fd_binary_get_varuint(&binary);
        uint32_t content_length = (uint32_t)fd_binary_get_varuint(&binary);
        uint32_t content_index = binary.get_index;
        if (binary.flags == 0) {
            fdi_api_dispatch(identifier, type, &binary);
        } else {
            fd_log_assert_fail("underflow");
        }
        fd_log_assert(binary.get_index <= (content_index + content_length));
        binary.get_index = content_index + content_length;
    } while (binary.get_index < binary.size);

    primask = fdi_interrupt_disable();
    fdi_api.rx_index += binary.size;
    // if buffer is empty and there isn't a transfer occurring reset the buffer
    if ((fdi_api.rx_index >= fdi_api.rx_length) && (fdi_api.merge.length == 0)) {
        fdi_api.rx_index = 0;
        fdi_api.rx_length = 0;
    }
    fdi_interrupt_restore(primask);
    return true;
}

void fdi_api_process(void) {
    while (fdi_api_process_tx());

    for (uint32_t i = 0; i < fdi_api.configuration.apic_count; ++i) {
        fdi_apic_t *apic = &fdi_api.configuration.apics[i];
        fdi_apic_response_t raw;
        if (!fdi_apic_rx(apic, &raw)) {
            continue;
        }
        fd_binary_t binary;
        fd_binary_initialize(&binary, (uint8_t *)raw.data, raw.count);
        uint64_t identifier = fd_binary_get_varuint(&binary);
        uint64_t type = fd_binary_get_varuint(&binary);
        if (binary.flags & FD_BINARY_FLAG_OVERFLOW) {
            continue;
        }
        if (identifier == 0) {
            if (type == 0) {
                // no response data pending
            } else
            if (type == 1 /* discover instruments */) {
                fdi_apic_discover_instruments_response(apic, raw.data, raw.count);
            } else {
                fd_log_assert_fail("unexpected instrument manager response");
            }
            continue;
        }
        uint8_t *data = &binary.buffer[binary.get_index];
        uint32_t count = binary.size - binary.get_index;
        fdi_api_send(identifier, type, data, count);
        while (fdi_api_process_tx());
    }

    while (fdi_api_process_rx());
}

// append data to tx buffer
bool fdi_api_send(uint64_t identifier, uint64_t type, uint8_t *data, uint32_t length) {
    uint8_t buffer[32];
    fd_binary_t binary;
    fd_binary_initialize(&binary, buffer, sizeof(buffer));
    fd_binary_put_varuint(&binary, identifier);
    fd_binary_put_varuint(&binary, type);
    fd_binary_put_varuint(&binary, length);
    uint32_t header_length = binary.put_index;
    uint32_t content_length = header_length + length;

    uint32_t buffer_index = fdi_api.tx_length;
    uint32_t data_index = 0;
    uint32_t data_remaining = length;
    uint64_t ordinal = 0;
    while ((ordinal == 0) || (data_remaining > 0)) {
        uint32_t free = fdi_api_tx_size - buffer_index;
        fd_binary_initialize(&binary, &fdi_api.tx_buffer[buffer_index], free);
        fd_binary_put_uint8(&binary, 0 /* length placeholder */);
        fd_binary_put_varuint(&binary, ordinal);
        if (ordinal == 0) {
            fd_binary_put_varuint(&binary, content_length);
            fd_binary_put_varuint(&binary, identifier);
            fd_binary_put_varuint(&binary, type);
            fd_binary_put_varuint(&binary, length);
        }
        uint32_t available = 1 + fdi_api_send_limit - binary.put_index;
        uint32_t amount = data_remaining;
        if (amount > available) {
            amount = available;
        }
        fd_binary_put_bytes(&binary, &data[data_index], amount);
        if (binary.flags & FD_BINARY_FLAG_OVERFLOW) {
            fdi_api.tx_index = 0;
            fdi_api.tx_length = 0;
            return false;
        }
        data_index += amount;
        data_remaining -= amount;
        fdi_api.tx_buffer[buffer_index] = binary.put_index - 1;
        buffer_index += binary.put_index;
        ordinal += 1;
    }

    fdi_api.tx_length = buffer_index;
    return true;
}

void fdi_api_merge_clear(void) {
    memset(&fdi_api.merge, 0, sizeof(fdi_api.merge));
}

// called from interrupt context
void fdi_api_tx_callback(void) {
    // wakeup and send next chunk
    // !!! need to implement to add wait to main loop -denis
}

// called from interrupt context
// append data to rx buffer
void fdi_api_rx_callback(uint8_t *data, uint32_t length) {
    uint32_t primask = fdi_interrupt_disable();
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint64_t ordinal = fd_binary_get_varuint(&binary);
    if (ordinal == 0) {
        // start of sequence
        fd_log_assert(fdi_api.merge.ordinal == 0);
        fdi_api_merge_clear();
        fdi_api.merge.length = (uint32_t)fd_binary_get_varuint(&binary);
        fd_log_assert(fdi_api.merge.length < fdi_api_rx_size);
        fd_binary_t binary_rx;
        fd_binary_initialize(&binary_rx, &fdi_api.rx_buffer[fdi_api.rx_length], fdi_api_rx_size - fdi_api.rx_length);
        fd_binary_put_uint16(&binary_rx, fdi_api.merge.length);
    } else {
        if (ordinal != (fdi_api.merge.ordinal + 1)) {
            fd_log_assert_fail("out of sequence");
            fdi_api_merge_clear();
            goto done;
        }
        fdi_api.merge.ordinal += 1;
    }

    uint8_t *content_data = &data[binary.get_index];
    uint32_t content_length = length - binary.get_index;

    uint32_t rx_free = fdi_api_rx_size - (fdi_api.rx_length + 2 + fdi_api.merge.offset + content_length);
    if (content_length > rx_free) {
        fd_log_assert_fail("overflow");
        fdi_api_merge_clear();
        goto done;
    }

    memcpy(&fdi_api.rx_buffer[fdi_api.rx_length + 2 + fdi_api.merge.offset], content_data, content_length);
    fdi_api.merge.offset += content_length;

    if (fdi_api.merge.offset >= fdi_api.merge.length) {
        // merge complete
        fdi_api.rx_length += 2 + fdi_api.merge.length;
        fdi_api_merge_clear();
    }

done:
    fdi_interrupt_restore(primask);
}

const fdi_api_configuration_t *fdi_api_get_configuration(void) {
    return &fdi_api.configuration;
}

void fdi_api_initialize(fdi_api_configuration_t configuration) {
    memset(&fdi_api, 0, sizeof(fdi_api));
    fdi_api.configuration = configuration;
}
