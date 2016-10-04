#include "fdi_api.h"

#include "fdi_interrupt.h"
#include "fdi_usb.h"

#include "fd_binary.h"
#include "fd_log.h"

#include <string.h>

#define fdi_api_usb_send_limit 64

typedef struct {
    uint32_t ordinal;
    uint32_t length;
    uint32_t offset;
} fdi_api_merge_t;

fdi_api_merge_t fdi_api_merge;

typedef struct {
    uint64_t identifier;
    uint64_t type;
    fdi_api_function_t function;
} fdi_api_entry_t;

#define fdi_api_entry_size 256
fdi_api_entry_t fdi_api_entrys[fdi_api_entry_size];
uint32_t fdi_api_entry_count;

uint32_t fdi_api_tx_index;
uint32_t fdi_api_tx_length;
#define fdi_api_tx_size 5120
uint8_t fdi_api_tx_buffer[fdi_api_tx_size];

uint32_t fdi_api_rx_index;
uint32_t fdi_api_rx_length;
#define fdi_api_rx_size 5120
uint8_t fdi_api_rx_buffer[fdi_api_rx_size];

fdi_api_can_transmit_handler_t fdi_api_can_transmit_handler;
fdi_api_transmit_handler_t fdi_api_transmit_handler;
fdi_api_dispatch_handler_t fdi_api_dispatch_handler;

void fdi_api_register(uint64_t identifier, uint64_t type, fdi_api_function_t function) {
    fd_log_assert(fdi_api_entry_count < fdi_api_entry_size);
    fdi_api_entry_t *entry = &fdi_api_entrys[fdi_api_entry_count];
    entry->identifier = identifier;
    entry->type = type;
    entry->function = function;
    fdi_api_entry_count += 1;
}

fdi_api_function_t fdi_api_lookup(uint64_t identifier, uint64_t type) {
    for (uint32_t i = 0; i < fdi_api_entry_count; ++i) {
        fdi_api_entry_t *entry = &fdi_api_entrys[i];
        if ((entry->identifier == identifier) && (entry->type == type)) {
            return entry->function;
        }
    }
    return 0;
}

bool fdi_api_can_transmit(void) {
    return fdi_usb_can_send();
}

void fdi_api_transmit(uint8_t *data, uint32_t length) {
    fdi_usb_send(data, length);
}

int fdi_api_transmit_count = 0;

// remove data from tx buffer
bool fdi_api_process_tx(void) {
    if ((fdi_api_tx_length > 0) && fdi_api_can_transmit_handler()) {
        uint8_t length = fdi_api_tx_buffer[fdi_api_tx_index];
        fd_log_assert((fdi_api_tx_index + length) <= fdi_api_tx_length);
        fdi_api_tx_index += 1;
        ++fdi_api_transmit_count;
        fdi_api_transmit_handler(&fdi_api_tx_buffer[fdi_api_tx_index], length);
        fdi_api_tx_index += length;
        if (fdi_api_tx_index == fdi_api_tx_length) {
            fdi_api_tx_index = 0;
            fdi_api_tx_length = 0;
        }
        return true;
    }
    return false;
}

typedef struct {
    uint64_t identifier;
    uint64_t type;
} fdi_api_event_t;

fdi_api_event_t fdi_api_event_history[256];
uint32_t fdi_api_event_history_index = 0;

void fdi_api_dispatch(uint64_t identifier, uint64_t type, fd_binary_t *binary) {
    fdi_api_event_t *event = &fdi_api_event_history[fdi_api_event_history_index++ & 0xff];
    event->identifier = identifier;
    event->type = type;
    
    fdi_api_function_t function = fdi_api_lookup(identifier, type);
    if (function) {
        function(identifier, type, binary);
        fd_log_assert(binary->flags == 0);
    } else {
        fd_log_assert_fail("unknown function");
    }
}

// remove data from rx buffer
bool fdi_api_process_rx(void) {
    uint32_t primask = fdi_interrupt_disable();
    uint32_t index = fdi_api_rx_index;
    uint32_t length = fdi_api_rx_length;
    fdi_interrupt_restore(primask);

    if (index >= length) {
        return false;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, &fdi_api_rx_buffer[index], length - index);
    uint32_t function_length = (uint32_t)fd_binary_get_uint16(&binary);
    do {
        uint64_t identifier = fd_binary_get_varuint(&binary);
        uint64_t type = fd_binary_get_varuint(&binary);
        uint32_t content_length = (uint32_t)fd_binary_get_varuint(&binary);
        uint32_t content_index = binary.get_index;
        if (binary.flags == 0) {
            fdi_api_dispatch_handler(identifier, type, &binary);
        } else {
            fd_log_assert_fail("underflow");
        }
        fd_log_assert(binary.get_index <= (content_index + content_length));
        binary.get_index = content_index + content_length;
    } while (binary.get_index < binary.size);

    primask = fdi_interrupt_disable();
    fdi_api_rx_index += 2 + function_length;
    // if buffer is empty and there isn't a transfer occurring reset the buffer
    if ((fdi_api_rx_index >= fdi_api_rx_length) && (fdi_api_merge.length == 0)) {
        fdi_api_rx_index = 0;
        fdi_api_rx_length = 0;
    }
    fdi_interrupt_restore(primask);
    return true;
}

void fdi_api_process(void) {
    while (fdi_api_process_tx());
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

    uint32_t buffer_index = fdi_api_tx_length;
    uint32_t data_index = 0;
    uint32_t data_remaining = length;
    uint64_t ordinal = 0;
    while ((ordinal == 0) || (data_remaining > 0)) {
        uint32_t free = fdi_api_tx_size - buffer_index;
        fd_binary_initialize(&binary, &fdi_api_tx_buffer[buffer_index], free);
        fd_binary_put_uint8(&binary, 0 /* length placeholder */);
        fd_binary_put_varuint(&binary, ordinal);
        if (ordinal == 0) {
            fd_binary_put_varuint(&binary, content_length);
            fd_binary_put_varuint(&binary, identifier);
            fd_binary_put_varuint(&binary, type);
            fd_binary_put_varuint(&binary, length);
        }
        uint32_t available = 1 + fdi_api_usb_send_limit - binary.put_index;
        uint32_t amount = data_remaining;
        if (amount > available) {
            amount = available;
        }
        fd_binary_put_bytes(&binary, &data[data_index], amount);
        if (binary.flags & FD_BINARY_FLAG_OVERFLOW) {
            return false;
        }
        data_index += amount;
        data_remaining -= amount;
        fdi_api_tx_buffer[buffer_index] = binary.put_index - 1;
        buffer_index += binary.put_index;
        ordinal += 1;
    }

    fdi_api_tx_length = buffer_index;
    return true;
}

void fdi_api_merge_clear(void) {
    memset(&fdi_api_merge, 0, sizeof(fdi_api_merge));
}

// called from interrupt context
void fdi_api_tx_callback(void) {
    // wakeup and send next chunk
    // !!! need to implement to add wait to main loop -denis
}

// called from interrupt context
// append data to rx buffer
void fdi_api_rx_callback(uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint64_t ordinal = fd_binary_get_varuint(&binary);
    if (ordinal == 0) {
        // start of sequence
        fd_log_assert(fdi_api_merge.ordinal == 0);
        fdi_api_merge_clear();
        fdi_api_merge.length = (uint32_t)fd_binary_get_varuint(&binary);
        fd_log_assert(fdi_api_merge.length < fdi_api_rx_size);
        fd_binary_t binary_rx;
        fd_binary_initialize(&binary_rx, &fdi_api_rx_buffer[fdi_api_rx_length], fdi_api_rx_size - fdi_api_rx_length);
        fd_binary_put_uint16(&binary_rx, fdi_api_merge.length);
    } else {
        if (ordinal != (fdi_api_merge.ordinal + 1)) {
            fd_log_assert_fail("out of sequence");
            fdi_api_merge_clear();
            return;
        }
        fdi_api_merge.ordinal += 1;
    }

    uint8_t *content_data = &data[binary.get_index];
    uint32_t content_length = length - binary.get_index;

    uint32_t rx_free = fdi_api_rx_size - (fdi_api_rx_length + 2 + fdi_api_merge.offset + content_length);
    if (content_length > rx_free) {
        fd_log_assert_fail("overflow");
        fdi_api_merge_clear();
        return;
    }

    memcpy(&fdi_api_rx_buffer[fdi_api_rx_length + 2 + fdi_api_merge.offset], content_data, content_length);
    fdi_api_merge.offset += content_length;

    if (fdi_api_merge.offset >= fdi_api_merge.length) {
        // merge complete
        fdi_api_rx_length += 2 + fdi_api_merge.length;
        fdi_api_merge_clear();
    }
}

void fdi_api_initialize(void) {
    fdi_api_entry_count = 0;

    fdi_api_tx_index = 0;
    fdi_api_tx_length = 0;

    fdi_api_rx_index = 0;
    fdi_api_rx_length = 0;

    fdi_api_can_transmit_handler = fdi_api_can_transmit;
    fdi_api_transmit_handler = fdi_api_transmit;
    fdi_api_dispatch_handler = fdi_api_dispatch;
}

void fdi_api_initialize_usb(void) {
    fdi_usb_set_data_callback(fdi_api_rx_callback);
    fdi_usb_set_tx_ready_callback(fdi_api_tx_callback);
}