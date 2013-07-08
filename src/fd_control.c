#include "fd_binary.h"
#include "fd_control.h"
#include "fd_control_codes.h"
#include "fd_indicator.h"
#include "fd_map.h"
#include "fd_power.h"
#include "fd_processor.h"
#include "fd_rtc.h"
#include "fd_sync.h"

#include <em_msc.h>

#include <string.h>

typedef void (*fd_control_command_t)(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length);

#define DETOUR_BUFFER_SIZE 200

fd_detour_source_t fd_control_detour_source;
uint8_t fd_control_detour_buffer[DETOUR_BUFFER_SIZE];

void fd_control_initialize(void) {
    fd_detour_source_initialize(&fd_control_detour_source);
}

void fd_control_detour_supplier(uint32_t offset, uint8_t *data, uint32_t length) {
    memcpy(data, &fd_control_detour_buffer[offset], length);
}

#define USER_DATA_ADDRESS 0x0fe00000
// user data is 2kB

#define LOCK_BITS_ADDRESS 0x0FE04000
#define DEBUG_LOCK_WORD_ADDRESS (LOCK_BITS_ADDRESS + 127)

bool fd_get_debug_lock(void) {
    uint32_t *address = (uint32_t*)DEBUG_LOCK_WORD_ADDRESS;
    uint32_t debug_lock = *address;
    return (debug_lock & 0xf) != 0xf;
}

void fd_set_debug_lock(void) {
    if (!fd_get_debug_lock()) {
        uint32_t word = 0xfffffff0;
         MSC_Init();
        __disable_irq();
        MSC_WriteWord((uint32_t*)DEBUG_LOCK_WORD_ADDRESS, &word, 4);
        __enable_irq();
        MSC_Deinit();
    }
}

// provisioning format:
// - uint16_t version
// - uint16_t flags;
// - uint8_t[16] AES key
// - map

#define PROVISION_OPTION_DEBUG_LOCK 0x00000001
#define PROVISION_OPTION_RESET 0x00000002

typedef struct {
    uint16_t version;
    uint16_t flags;
    uint8_t key[16];
    // map follows...
} fd_provision_t;

#define PROVISION_MAP_ADDRESS (USER_DATA_ADDRESS + 20)

void fd_control_provision(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint32_t options = fd_binary_get_uint32(&binary);
    uint32_t provision_data_length = fd_binary_get_uint16(&binary);
    uint8_t *provision_data = &binary.buffer[binary.get_index];

    uint32_t *address = (uint32_t*)USER_DATA_ADDRESS;
    MSC_Init();
    __disable_irq();
    MSC_ErasePage(address);
    MSC_WriteWord(address, provision_data, provision_data_length);
    __enable_irq();
    MSC_Deinit();

    if (options & PROVISION_OPTION_DEBUG_LOCK) {
        fd_set_debug_lock();
    }
    if (options & PROVISION_OPTION_RESET) {
        NVIC_SystemReset();
    }
}

#define FD_CONTROL_RESET_SYSTEM_REQUEST 0
#define FD_CONTROL_RESET_WATCHDOG 1
#define FD_CONTROL_RESET_HARD_FAULT 2

void fd_control_reset(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint8_t type = fd_binary_get_uint8(&binary);
    switch (type) {
        case FD_CONTROL_RESET_SYSTEM_REQUEST: {
            NVIC_SystemReset();
        } break;
        case FD_CONTROL_RESET_WATCHDOG: {
            fd_delay_ms(10000);
        } break;
        case FD_CONTROL_RESET_HARD_FAULT: {
            void (*null_fn)(void) = 0;
            (*null_fn)();
        } break;
        default: break;
    }
}

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 0
#define VERSION_CAPABILITIES 0x00000000

// !!! should come from gcc command line define
#define GIT_COMMIT 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19

static
uint8_t git_commit[20] = {GIT_COMMIT};

void fd_control_get_property_version(fd_binary_t *binary) {
    fd_binary_put_uint16(binary, VERSION_MAJOR);
    fd_binary_put_uint16(binary, VERSION_MINOR);
    fd_binary_put_uint16(binary, VERSION_PATCH);
    fd_binary_put_uint32(binary, VERSION_CAPABILITIES);
    fd_binary_put_bytes(binary, git_commit, sizeof(git_commit));
}

void fd_control_get_property_site(fd_binary_t *binary) {
    uint8_t *site;
    uint16_t site_length;
    fd_map_get("site", FD_MAP_TYPE_UTF8, &site, &site_length);

    fd_binary_put_uint16(binary, site_length);
    fd_binary_put_bytes(binary, site, site_length);
}

void fd_control_get_property_debug_lock(fd_binary_t *binary) {
    fd_binary_put_uint8(binary, fd_get_debug_lock());
}

void fd_control_set_property_debug_lock(fd_binary_t *binary __attribute__((unused))) {
    fd_set_debug_lock();
}

void fd_control_get_property_rtc(fd_binary_t *binary) {
    fd_time_t time = fd_rtc_get_time();
    fd_binary_put_time64(binary, time);
}

void fd_control_set_property_rtc(fd_binary_t *binary) {
    fd_time_t time = fd_binary_get_time64(binary);
    fd_rtc_set_time(time);
}

void fd_control_get_property_power(fd_binary_t *binary) {
    fd_power_t power;
    fd_power_get(&power);
    fd_binary_put_float32(binary, power.battery_level);
    fd_binary_put_float32(binary, power.battery_voltage);
    fd_binary_put_uint8(binary, power.is_usb_powered);
    fd_binary_put_uint8(binary, power.is_charging);
    fd_binary_put_float32(binary, power.charge_current);
    fd_binary_put_float32(binary, power.temperature);
}

void fd_control_set_property_power(fd_binary_t *binary) {
    float battery_level = fd_binary_get_float32(binary);
    fd_power_set(battery_level);
}

void fd_control_get_properties(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);

    fd_binary_initialize(&binary, fd_control_detour_buffer, DETOUR_BUFFER_SIZE);
    fd_binary_put_uint8(&binary, FD_CONTROL_GET_PROPERTIES);
    fd_binary_put_uint32(&binary, properties);
    for (uint32_t property = 1; property != 0; property <<= 1) {
        if (property & properties) {
            switch (property) {
                case FD_CONTROL_PROPERTY_VERSION: {
                    fd_control_get_property_version(&binary);
                } break;
                case FD_CONTROL_PROPERTY_DEBUG_LOCK: {
                    fd_control_get_property_debug_lock(&binary);
                } break;
                case FD_CONTROL_PROPERTY_RTC: {
                    fd_control_get_property_rtc(&binary);
                } break;
                case FD_CONTROL_PROPERTY_POWER: {
                    fd_control_get_property_power(&binary);
                } break;
                case FD_CONTROL_PROPERTY_SITE: {
                    fd_control_get_property_site(&binary);
                } break;
            }
        }
    }

    uint32_t sync_length = binary.put_index;

    fd_detour_source_set(&fd_control_detour_source, fd_control_detour_supplier, sync_length);
    fd_detour_source_collection_push(detour_source_collection, &fd_control_detour_source);
}

void fd_control_set_properties(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);
    for (uint32_t property = 1; property != 0; property <<= 1) {
        if (property & properties) {
            switch (property) {
                case FD_CONTROL_PROPERTY_DEBUG_LOCK: {
                    fd_control_set_property_debug_lock(&binary);
                } break;
                case FD_CONTROL_PROPERTY_RTC: {
                    fd_control_set_property_rtc(&binary);
                } break;
                case FD_CONTROL_PROPERTY_POWER: {
                    fd_control_set_property_power(&binary);
                } break;
            }
        }
    }
}

static
void get_rgb(fd_binary_t *binary, fd_indicator_rgb_t *rgb) {
    rgb->r = fd_binary_get_uint8(binary);
    rgb->g = fd_binary_get_uint8(binary);
    rgb->b = fd_binary_get_uint8(binary);
}

void fd_control_indicator_override(fd_detour_source_collection_t *detour_source_collection __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_indicator_state_t state;
    state.usb.o = fd_binary_get_uint8(&binary);
    state.usb.g = fd_binary_get_uint8(&binary);
    state.d0.r = fd_binary_get_uint8(&binary);
    get_rgb(&binary, &state.d1);
    get_rgb(&binary, &state.d2);
    get_rgb(&binary, &state.d3);
    state.d4.r = fd_binary_get_uint8(&binary);
    fd_time_t duration = fd_binary_get_time64(&binary);

    fd_indicator_override(&state, duration);
}

// !!! should we encrypt/decrypt everything? or just syncs? or just things that modify? -denis

void fd_control_process(fd_detour_source_collection_t *detour_source_collection, uint8_t *data, uint32_t length) {
    if (length < 1) {
        return;
    }
    uint8_t code = data[0];
    fd_control_command_t command = 0;
    switch (code) {
        case FD_CONTROL_GET_PROPERTIES:
            command = fd_control_get_properties;
            break;
        case FD_CONTROL_SET_PROPERTIES:
            command = fd_control_set_properties;
            break;

        case FD_CONTROL_PROVISION:
            command = fd_control_provision;
            break;
        case FD_CONTROL_RESET:
            command = fd_control_reset;
            break;

        case FD_CONTROL_SYNC_START:
            command = fd_sync_start;
            break;
        case FD_CONTROL_SYNC_ACK:
            command = fd_sync_ack;
            break;

        case FD_CONTROL_INDICATOR_OVERRIDE:
            command = fd_control_indicator_override;
            break;
    }
    if (command) {
        (*command)(detour_source_collection, &data[1], length - 1);
    }
}