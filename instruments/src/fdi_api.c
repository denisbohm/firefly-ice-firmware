#include "fdi_api.h"

#include "fdi_detour.h"
#include "fdi_interrupt.h"
#include "fdi_usb.h"

#include "fd_binary.h"
#include "fd_detour.h"
#include "fd_log.h"

#include <string.h>

typedef struct {
    uint32_t type;
    fdi_api_function_t function;
} fdi_api_entry_t;

#define fdi_api_entry_size 64
fdi_api_entry_t fdi_api_entrys[fdi_api_entry_size];
uint32_t fdi_api_entry_count;

fd_detour_t fdi_api_usb_detour_source;
#define fdi_api_usb_detour_source_size 300
uint8_t fdi_api_usb_detour_source_data[fdi_api_usb_detour_source_size];

fd_detour_t fdi_api_usb_detour;
#define fdi_api_usb_detour_size 300
uint8_t fdi_api_usb_detour_data[fdi_api_usb_detour_size];

uint8_t fdi_api_data[fdi_api_usb_detour_size];
volatile uint32_t fdi_api_data_length;
volatile uint32_t fdi_api_data_overrun;

static bool rx = false;

static void fdi_api_data_callback(uint8_t *data, size_t length) {
    rx = true;
}

void fdi_api_initialize(void) {
    fdi_api_entry_count = 0;

    fdi_detour_source_initialize(&fdi_api_usb_detour_source, fdi_api_usb_detour_source_data, fdi_api_usb_detour_source_size);

    fd_detour_initialize(&fdi_api_usb_detour, fdi_api_usb_detour_data, fdi_api_usb_detour_size);

    fdi_api_data_length = 0;
    fdi_api_data_overrun = 0;
}

void fdi_api_initialize_usb(void) {
    fdi_usb_set_data_callback(fdi_api_data_callback);
}

void fdi_api_register(uint32_t type, fdi_api_function_t function) {
    fd_log_assert(fdi_api_entry_count < fdi_api_entry_size);
    fdi_api_entry_t *entry = &fdi_api_entrys[fdi_api_entry_count];
    entry->type = type;
    entry->function = function;
    fdi_api_entry_count += 1;
}

fdi_api_function_t fdi_api_lookup(uint32_t type) {
    for (uint32_t i = 0; i < fdi_api_entry_count; ++i) {
        fdi_api_entry_t *entry = &fdi_api_entrys[i];
        if (entry->type == type) {
            return entry->function;
        }
    }
    return 0;
}

void fdi_api_process(void) {
    if (fdi_api_usb_detour_source.state != fd_detour_state_clear) {
        uint8_t data[64];
        if (fdi_detour_source_get(&fdi_api_usb_detour_source, data, sizeof(data))) {
            fdi_usb_send(data, sizeof(data));
        }
    }

    uint32_t primask = fdi_interrupt_disable();
    uint32_t length = fdi_api_data_length;
    fdi_interrupt_restore(primask);

    if (length == 0) {
        return;
    }

    fd_binary_t binary;
    fd_binary_initialize(&binary, fdi_api_data, length);
    uint32_t type = fd_binary_get_uint32(&binary);
    fdi_api_function_t function = fdi_api_lookup(type);
    if (function) {
        function(&binary);
    } else {
        fd_log_assert_fail("");
    }

    primask = fdi_interrupt_disable();
    fdi_api_data_length = 0;
    fdi_interrupt_restore(primask);
}

void fdi_api_send(uint32_t type, uint8_t *data, uint32_t length) {
    fd_log_assert(fdi_api_usb_detour_source.state == fd_detour_state_clear);
    
    fd_binary_t binary;
    fd_binary_initialize(&binary, fdi_api_usb_detour_source_data, fdi_api_usb_detour_source_size);
    fd_binary_put_uint32(&binary, type);
    fd_binary_put_bytes(&binary, data, length);
    fdi_detour_source_set(&fdi_api_usb_detour_source, binary.put_index);
}

// called from interrupt context
void fdi_api_enqueue(fd_detour_t *detour) {
    if (fdi_api_data_length > 0) {
        ++fdi_api_data_overrun;
    } else {
        memcpy(fdi_api_data, detour->data, detour->length);
        fdi_api_data_length = detour->length;
    }
}

// called from interrupt context
void fdi_api_event(uint8_t *data, uint32_t length) {
    fd_detour_event(&fdi_api_usb_detour, data, length);
    switch (fd_detour_state(&fdi_api_usb_detour)) {
        case fd_detour_state_clear:
        case fd_detour_state_intermediate:
        break;
        case fd_detour_state_success:
            fdi_api_enqueue(&fdi_api_usb_detour);
            fd_detour_clear(&fdi_api_usb_detour);
        break;
        case fd_detour_state_error:
            fd_log_assert_fail("");
            fd_detour_clear(&fdi_api_usb_detour);
        break;
    }
}
