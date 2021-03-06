#include "fd_binary.h"
#include "fd_bluetooth.h"
#include "fd_control.h"
#include "fd_control_codes.h"
#include "fd_event.h"
#include "fd_hal_aes.h"
#include "fd_hal_ble.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_hal_rtc.h"
#include "fd_hal_system.h"
#include "fd_hal_ui.h"
#include "fd_hal_usb.h"
#include "fd_lock.h"
#include "fd_log.h"
#include "fd_main.h"
#include "fd_map.h"
#include "fd_packet.h"
#include "fd_power.h"
#include "fd_provision.h"
#include "fd_recognition.h"
#include "fd_sensing.h"
#include "fd_sha.h"
#include "fd_storage.h"
#include "fd_sync.h"
#include "fd_update.h"

#include <string.h>

#define COMMAND_BUFFER_SIZE 1024

uint8_t fd_control_command_buffer[COMMAND_BUFFER_SIZE];

#define INPUT_BUFFER_SIZE 1024

uint8_t fd_control_input_buffer[INPUT_BUFFER_SIZE];
uint32_t fd_control_input_buffer_count;

typedef struct {
    fd_packet_output_t *packet_output;
    uint32_t length;
} fd_control_input_t;

#define INPUTS_SIZE 16

fd_control_input_t fd_control_inputs[INPUTS_SIZE];
uint32_t fd_control_inputs_count;

#define FD_CONTROL_OUTPUT_BUFFER_SIZE 1024

uint8_t fd_control_output_buffer[FD_CONTROL_OUTPUT_BUFFER_SIZE];
fd_binary_t fd_control_output_binary;

void fd_control_initialize_commands(void);
void fd_control_command(void);
void fd_control_check_notify_properties(void);

fd_control_command_t fd_control_commands[256];

fd_control_callback_t fd_control_before_callback;
fd_control_callback_t fd_control_after_callback;

typedef struct {
    fd_control_get_property_fn_t get;
    fd_control_set_property_fn_t set;
} fd_control_property_t;

fd_control_property_t fd_control_propertys[32];

void fd_control_initialize(void) {
    fd_control_input_buffer_count = 0;
    fd_control_inputs_count = 0;

    memset(fd_control_commands, 0, sizeof(fd_control_commands));
    fd_control_initialize_commands();

    fd_event_add_callback(FD_EVENT_COMMAND, fd_control_command);
    fd_event_add_callback(FD_EVENT_NOTIFY, fd_control_check_notify_properties);

    fd_control_before_callback = 0;
    fd_control_after_callback = 0;
}

void fd_control_set_command(uint8_t code, fd_control_command_t command) {
    fd_control_commands[code] = command;
}

void fd_control_process(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    if (fd_control_inputs_count >= INPUTS_SIZE) {
        return;
    }
    if ((fd_control_input_buffer_count + length) >= INPUT_BUFFER_SIZE) {
        return;
    }

    fd_control_input_t *input = &fd_control_inputs[fd_control_inputs_count];
    input->packet_output = packet_output;
    input->length = length;

    memcpy(&fd_control_input_buffer[fd_control_input_buffer_count], data, length);
    fd_control_input_buffer_count += length;

    ++fd_control_inputs_count;

    fd_event_set(FD_EVENT_COMMAND);
}

fd_binary_t *fd_control_send_start(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t type) {
    fd_binary_initialize(&fd_control_output_binary, fd_control_output_buffer, sizeof(fd_control_output_buffer));
    fd_binary_put_uint8(&fd_control_output_binary, type);
    return &fd_control_output_binary;
}

bool fd_control_send_complete(fd_packet_output_t *packet_output) {
    return packet_output->write(fd_control_output_binary.buffer, fd_control_output_binary.put_index);
}

void fd_control_ping(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint16_t ping_length = fd_binary_get_uint16(&binary);
    uint8_t *ping_data = &binary.buffer[binary.get_index];

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_PING);
    fd_binary_put_uint16(binary_out, ping_length);
    fd_binary_put_bytes(binary_out, ping_data, ping_length);
    fd_control_send_complete(packet_output);
}

void fd_control_rtc(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t flags = fd_binary_get_uint32(&binary);
    if (flags & FD_CONTROL_RTC_FLAG_SET_TIME) {
        fd_time_t time;
        time.seconds = fd_binary_get_uint32(&binary);
        time.microseconds = fd_binary_get_uint32(&binary);
        fd_hal_rtc_set_time(time);
    }
    if (flags & FD_CONTROL_RTC_FLAG_SET_UTC_OFFSET) {
        int32_t utc_offset = fd_binary_get_uint32(&binary);
        fd_hal_rtc_set_utc_offset(utc_offset);
    }

    if (flags & (FD_CONTROL_RTC_FLAG_GET_TIME | FD_CONTROL_RTC_FLAG_GET_UTC_OFFSET)) {
        fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_RTC);
        fd_binary_put_uint32(binary_out, flags);
        if (flags & FD_CONTROL_RTC_FLAG_GET_TIME) {
            fd_time_t time = fd_hal_rtc_get_time();
            fd_binary_put_uint32(binary_out, time.seconds);
            fd_binary_put_uint32(binary_out, time.microseconds);
        }
        if (flags & FD_CONTROL_RTC_FLAG_GET_UTC_OFFSET) {
            int32_t utc_offset = fd_hal_rtc_get_utc_offset();
            fd_binary_put_uint32(binary_out, utc_offset);
        }
        fd_control_send_complete(packet_output);
    }
}

void fd_control_hardware(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t flags = fd_binary_get_uint32(&binary);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_HARDWARE);
    fd_binary_put_uint32(binary_out, flags);
    if (flags & FD_CONTROL_HARDWARE_FLAG_GET_UNIQUE) {
        fd_hal_processor_get_hardware_unique(binary_out);
    }
    if (flags & FD_CONTROL_HARDWARE_FLAG_GET_USB) {
        fd_binary_put_uint16(binary_out, fd_hal_usb_get_vendor_id());
        fd_binary_put_uint16(binary_out, fd_hal_usb_get_product_id());
    }
    if (flags & FD_CONTROL_HARDWARE_FLAG_GET_BLE) {
        fd_hal_ble_get_primary_service_uuid(binary_out);
    }
    if (flags & FD_CONTROL_HARDWARE_FLAG_GET_MODEL) {
        fd_binary_put_uint32(binary_out, fd_hal_processor_get_model_number());
    }
    fd_control_send_complete(packet_output);
}

void fd_control_provision(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint32_t options = fd_binary_get_uint32(&binary);
    uint32_t provision_data_length = fd_binary_get_uint16(&binary);
    uint8_t *provision_data = &binary.buffer[binary.get_index];

    if (provision_data_length > 0) {
        fd_hal_processor_write_user_data(provision_data, provision_data_length);
    }

#ifndef FD_NO_SENSING
    if (options & FD_PROVISION_OPTION_SENSING_ERASE) {
        fd_sensing_erase();
    }
#endif
    if (options & FD_PROVISION_OPTION_DEBUG_LOCK) {
        fd_hal_processor_set_debug_lock();
    }
    if (options & FD_PROVISION_OPTION_RESET) {
        fd_hal_reset_by(FD_HAL_RESET_SYSTEM_REQUEST);
    }
}

uint32_t fd_control_provision_get_utf8(const char *key, uint8_t **value) {
    uint16_t length = 0;
    uint8_t *map = fd_hal_processor_get_provision_map_address();
    if (map != 0) {
        fd_map_get(map, key, FD_MAP_TYPE_UTF8, value, &length);
    }
    return length;
}

bool fd_control_provision_put_utf8(const char *key, uint8_t *value, uint16_t value_length) {
    uint8_t *map = fd_hal_processor_get_provision_map_address();
    if (map != 0) {
        uint8_t buffer[128];
        fd_provision_t provision;
        provision.version = FD_PROVISION_VERSION;
        provision.flags = 0;
        memset(provision.key, 0, FD_HAL_AES_KEY_SIZE);
        memcpy(buffer, &provision, sizeof(provision));
        uint8_t *new_map = &buffer[sizeof(provision)];
        uint16_t new_map_length = sizeof(buffer) - sizeof(provision);
        if (fd_map_put(map, new_map, &new_map_length, key, FD_MAP_TYPE_UTF8, value, value_length)) {
            fd_hal_processor_write_user_data(buffer, sizeof(provision) + new_map_length);
            return true;
        }
    }
    return false;
}

uint32_t fd_control_get_name(uint8_t **name) {
    return fd_control_provision_get_utf8("name", name);
}

void fd_control_reset(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);

    uint8_t type = fd_binary_get_uint8(&binary);
    fd_hal_reset_by(type);
}

static
void fd_control_get_property_version(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_version_revision_t version;
    fd_hal_system_get_firmware_version(FD_HAL_SYSTEM_AREA_APPLICATION, &version);
    fd_binary_put_uint16(binary, version.major);
    fd_binary_put_uint16(binary, version.minor);
    fd_binary_put_uint16(binary, version.patch);
    fd_binary_put_uint32(binary, version.capabilities);
    fd_binary_put_bytes(binary, version.commit, FD_VERSION_COMMIT_SIZE);
}

static
void fd_control_get_property_hardware_id(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_hal_processor_get_hardware_id(binary);
}

static
void fd_control_get_property_site(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint8_t *site = 0;
    uint16_t site_length = 0;
    uint8_t *address = fd_hal_processor_get_provision_map_address();
    if (address != 0) {
        fd_map_get(address, "site", FD_MAP_TYPE_UTF8, &site, &site_length);
    }

    fd_binary_put_uint16(binary, site_length);
    fd_binary_put_bytes(binary, site, site_length);
}

static
void fd_control_get_property_reset(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint32(binary, fd_hal_reset_last.cause);
    fd_binary_put_time64(binary, fd_hal_reset_last.time);
}

static
void fd_control_get_property_retained(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint8(binary, fd_hal_reset_retained_was_valid_on_startup());
    fd_binary_put_uint32(binary, sizeof(fd_hal_reset_retained_at_initialize));
    fd_binary_put_bytes(binary, (uint8_t *)&fd_hal_reset_retained_at_initialize, sizeof(fd_hal_reset_retained_at_initialize));
}

static
void fd_control_get_property_storage(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint32(binary, fd_storage_used_page_count());
}

static
void fd_control_get_property_debug_lock(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint8(binary, fd_hal_processor_get_debug_lock());
}

static
void fd_control_set_property_debug_lock(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_hal_processor_set_debug_lock();
}

static
void fd_control_get_property_rtc(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_time_t time = fd_hal_rtc_get_time();
    fd_binary_put_time64(binary, time);
}

static
void fd_control_set_property_rtc(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_time_t time = fd_binary_get_time64(binary);
    fd_hal_rtc_set_time(time);
}

static
void fd_control_get_property_power(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_power_t power;
    fd_power_get(&power);
    fd_binary_put_float32(binary, power.battery_level);
    fd_binary_put_float32(binary, power.battery_voltage);
    fd_binary_put_uint8(binary, power.is_usb_powered);
    fd_binary_put_uint8(binary, power.is_charging);
    fd_binary_put_float32(binary, power.charge_current);
    fd_binary_put_float32(binary, power.temperature);
}

static
void fd_control_set_property_power(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    float battery_level = fd_binary_get_float32(binary);
    fd_power_set(battery_level);
}

static
void fd_control_get_property_mode(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint8(binary, fd_main_get_mode());
}

static
void fd_control_set_property_mode(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint8_t mode = fd_binary_get_uint8(binary);
    fd_main_set_mode(mode);
}

static
void fd_control_get_property_tx_power(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint8(binary, fd_bluetooth_get_tx_power());
}

static
void fd_control_set_property_tx_power(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint8_t level = fd_binary_get_uint8(binary);
    fd_bluetooth_set_tx_power(level);
}

static
void fd_control_get_property_boot_version(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_version_revision_t version;
    fd_hal_system_get_firmware_version(FD_HAL_SYSTEM_AREA_BOOTLOADER, &version);
    fd_binary_put_uint16(binary, version.major);
    fd_binary_put_uint16(binary, version.minor);
    fd_binary_put_uint16(binary, version.patch);
    fd_binary_put_uint32(binary, version.capabilities);
    fd_binary_put_bytes(binary, version.commit, FD_VERSION_COMMIT_SIZE);
}

static
void fd_control_get_property_regulator(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    bool switching = fd_hal_system_get_regulator();
    fd_binary_put_uint8(binary, switching ? 1 : 0);
}

static
void fd_control_set_property_regulator(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    bool switching = fd_binary_get_uint8(binary) != 0;
    fd_hal_system_set_regulator(switching);
}

static
void fd_control_get_property_logging(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint32(binary, FD_CONTROL_LOGGING_STATE | FD_CONTROL_LOGGING_COUNT);
    fd_binary_put_uint32(binary, fd_log_get_storage() ? FD_CONTROL_LOGGING_STORAGE : 0);
    fd_binary_put_uint32(binary, fd_log_get_count());
}

static
void fd_control_set_property_logging(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint32_t flags = fd_binary_get_uint32(binary);
    if (flags & FD_CONTROL_LOGGING_STATE) {
        uint32_t state = fd_binary_get_uint32(binary);
        fd_log_set_storage((state & FD_CONTROL_LOGGING_STORAGE) != 0);
    }
    if (flags & FD_CONTROL_LOGGING_COUNT) {
        uint32_t count = fd_binary_get_uint32(binary);
        fd_log_set_count(count);
    }
}

static
void fd_control_get_property_name(fd_binary_t *binary, fd_packet_output_t *packet_output) {
//    uint8_t *name;
//    uint8_t length = fd_control_provision_get_utf8("name", name);
    uint8_t name[20];
    uint8_t length = fd_bluetooth_get_name(name);
    fd_binary_put_uint8(binary, length);
    fd_binary_put_bytes(binary, name, length);
}

static void fd_control_set_name(uint8_t *data, uint32_t length) {
    bool result = fd_control_provision_put_utf8("name", data, length);
    fd_log_assert(result);

    fd_bluetooth_set_name(data, length);
}

static
void fd_control_set_property_name(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint8_t name[20];
    uint8_t length = fd_binary_get_uint8(binary);
    if (length > sizeof(name)) {
        length = sizeof(name);
    }
    fd_binary_get_bytes(binary, name, length);
    fd_control_set_name(name, length);
}

static
void fd_control_get_property_adc_vdd(fd_binary_t *binary, fd_packet_output_t *packet_output) {
//    fd_binary_put_float16(binary, fd_adc_get_vdd());
    fd_binary_put_float16(binary, fd_hal_system_get_regulated_voltage());
}

static
void fd_control_set_property_adc_vdd(fd_binary_t *binary, fd_packet_output_t *packet_output) {
//    fd_adc_set_vdd(fd_binary_get_float16(binary));
}

static
void fd_control_get_property_indicate(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    bool indicate = fd_hal_ui_get_indicate(packet_output->owner);
    fd_binary_put_uint8(binary, indicate ? 1 : 0);
}

static
void fd_control_set_property_indicate(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    bool indicate = fd_binary_get_uint8(binary) != 0;
    fd_hal_ui_set_indicate(packet_output->owner, indicate);
}

static
void fd_control_get_property_hardware_version(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_version_hardware_t version;
    fd_hal_system_get_hardware_version(&version);
    fd_binary_put_uint16(binary, version.major);
    fd_binary_put_uint16(binary, version.minor);
}

#ifndef FD_NO_SENSING

static
void fd_control_get_property_sensing_count(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint32_t count = fd_sensing_get_stream_sample_count();
    fd_binary_put_uint32(binary, count);
}

static
void fd_control_set_property_sensing_count(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint32_t count = fd_binary_get_uint32(binary);
    fd_sensing_set_stream_sample_count(count);
}

static
void fd_control_get_property_recognition(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint8(binary, fd_recognition_get_enable());
}

static
void fd_control_set_property_recognition(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    bool enable = fd_binary_get_uint8(binary) != 0;
    fd_recognition_set_enable(enable);
}

#endif

static
void fd_control_get_property_subscribe(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    fd_binary_put_uint32(binary, packet_output->subscribed_properties);
}

static
void fd_control_set_property_subscribe(fd_binary_t *binary, fd_packet_output_t *packet_output) {
    uint32_t properties = fd_binary_get_uint32(binary);
    packet_output->subscribed_properties = properties;
}

bool fd_control_send_properties(fd_packet_output_t *packet_output, uint8_t type, uint32_t properties) {
    fd_binary_t *binary_out = fd_control_send_start(packet_output, type);
    uint32_t mask = 0;
    for (uint32_t i = 0; i < 32; ++i) {
        if (properties & (1 << i)) {
            fd_control_property_t *property = &fd_control_propertys[i];
            if (property->get) {
                mask |= (1 << i);
            }
        }
    }
    fd_binary_put_uint32(binary_out, mask);
    for (uint32_t i = 0; i < 32; ++i) {
        if (mask & (1 << i)) {
            fd_control_property_t *property = &fd_control_propertys[i];
            if (property->get) {
                property->get(binary_out, packet_output);
            }
        }
    }
    return fd_control_send_complete(packet_output);
}

void fd_control_get_properties(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);
    fd_control_send_properties(packet_output, FD_CONTROL_GET_PROPERTIES, properties);
}

void fd_control_set_properties(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t properties = fd_binary_get_uint32(&binary);
    for (uint32_t i = 0; i < 32; ++i) {
        if (properties & (1 << i)) {
            fd_control_property_t *property = &fd_control_propertys[i];
            if (property->set) {
                property->set(&binary, packet_output);
            }
        }
    }
}

void fd_control_update_get_external_hash_impl(
    fd_packet_output_t *packet_output, uint8_t *data, uint32_t length, bool withArea
) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    uint32_t external_address = fd_binary_get_uint32(&binary);
    uint32_t external_length = fd_binary_get_uint32(&binary);
    uint8_t hash[FD_SHA_HASH_SIZE];
    fd_update_get_external_hash(area, external_address, external_length, hash);
    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_UPDATE_GET_EXTERNAL_HASH);
    fd_binary_put_bytes(binary_out, hash, FD_SHA_HASH_SIZE);
    fd_control_send_complete(packet_output);
}

void fd_control_update_get_sector_hashes_impl(
    fd_packet_output_t *packet_output, uint8_t *data, uint32_t length, bool withArea
) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    uint32_t sector_count = fd_binary_get_uint8(&binary);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_UPDATE_GET_SECTOR_HASHES);
    fd_binary_put_uint8(binary_out, sector_count);
    for (uint32_t i = 0; i < sector_count; ++i) {
        uint8_t hash[FD_SHA_HASH_SIZE];
        uint32_t sector = fd_binary_get_uint16(&binary);
        fd_update_get_sector_hash(area, sector, hash);
        fd_binary_put_uint16(binary_out, sector);
        fd_binary_put_bytes(binary_out, hash, FD_SHA_HASH_SIZE);
    }
    fd_control_send_complete(packet_output);
}

void fd_control_update_erase_sectors_impl(
    fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length, bool withArea
) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    uint32_t sector_count = fd_binary_get_uint8(&binary);
    for (uint32_t i = 0; i < sector_count; ++i) {
        uint32_t sector = fd_binary_get_uint16(&binary);
        fd_update_erase_sector(area, sector);
    }
}

void fd_control_update_write_page_impl(
    fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length, bool withArea
) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    uint32_t page = fd_binary_get_uint16(&binary);
    uint8_t *page_data = &binary.buffer[binary.get_index];
    fd_update_write_page(area, page, page_data);
}

void fd_control_update_read_page_impl(
    fd_packet_output_t *packet_output, uint8_t *data, uint32_t length, bool withArea
) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    uint32_t page = fd_binary_get_uint32(&binary);
    uint8_t page_data[256];
    fd_update_read_page(area, page, page_data);
    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_UPDATE_READ_PAGE);
    fd_binary_put_bytes(binary_out, page_data, 256);
    fd_control_send_complete(packet_output);
}

void fd_control_update_commit_impl(
    fd_packet_output_t *packet_output, uint8_t *data, uint32_t length, bool withArea
) {
    fd_version_metadata_t metadata;
    memset(&metadata, 0, sizeof(metadata));

    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = FD_HAL_SYSTEM_AREA_APPLICATION;
    if (withArea) {
        area = fd_binary_get_uint8(&binary);
    }
    metadata.binary.flags = fd_binary_get_uint32(&binary);
    metadata.binary.length = fd_binary_get_uint32(&binary);
    fd_binary_get_bytes(&binary, metadata.binary.hash, FD_SHA_HASH_SIZE);
    fd_binary_get_bytes(&binary, metadata.binary.crypt_hash, FD_SHA_HASH_SIZE);
    fd_binary_get_bytes(&binary, metadata.binary.crypt_iv, FD_VERSION_CRYPT_IV_SIZE);
    if (withArea) {
        metadata.revision.major = fd_binary_get_uint16(&binary);
        metadata.revision.minor = fd_binary_get_uint16(&binary);
        metadata.revision.patch = fd_binary_get_uint16(&binary);
        metadata.revision.capabilities = fd_binary_get_uint32(&binary);
        fd_binary_get_bytes(&binary, metadata.revision.commit, FD_VERSION_COMMIT_SIZE);
    }

    uint8_t result = fd_update_commit(area, &metadata);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_UPDATE_COMMIT);
    fd_binary_put_uint8(binary_out, result);
    fd_control_send_complete(packet_output);
}

///////

void fd_control_update_get_external_hash(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_get_external_hash_impl(packet_output, data, length, false);
}

void fd_control_update_get_sector_hashes(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_get_sector_hashes_impl(packet_output, data, length, false);
}

void fd_control_update_erase_sectors(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_erase_sectors_impl(packet_output, data, length, false);
}

void fd_control_update_write_page(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_write_page_impl(packet_output, data, length, false);
}

void fd_control_update_read_page(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_read_page_impl(packet_output, data, length, false);
}

void fd_control_update_commit(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_commit_impl(packet_output, data, length, false);
}

///////

void fd_control_update_area_get_version(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t area = fd_binary_get_uint8(&binary);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_UPDATE_AREA_GET_VERSION);
    uint32_t flags = 0;
    fd_version_revision_t revision;
    if (fd_hal_system_get_firmware_version(area, &revision)) {
        flags |= FD_CONTROL_UPDATE_AREA_GET_VERSION_FLAG_REVISION;
    }
    fd_version_metadata_t metadata;
    if (fd_hal_system_get_update_metadata(area, &metadata)) {
        flags |= FD_CONTROL_UPDATE_AREA_GET_VERSION_FLAG_METADATA;
    }
    fd_binary_put_uint32(binary_out, flags);

    if (flags & FD_CONTROL_UPDATE_AREA_GET_VERSION_FLAG_REVISION) {
        fd_binary_put_uint16(binary_out, revision.major);
        fd_binary_put_uint16(binary_out, revision.minor);
        fd_binary_put_uint16(binary_out, revision.patch);
        fd_binary_put_uint32(binary_out, revision.capabilities);
        fd_binary_put_bytes(binary_out, revision.commit, FD_VERSION_COMMIT_SIZE);
    }

    if (flags & FD_CONTROL_UPDATE_AREA_GET_VERSION_FLAG_METADATA) {
        fd_binary_put_uint32(binary_out, metadata.binary.flags);
        fd_binary_put_uint32(binary_out, metadata.binary.length);
        fd_binary_put_bytes(binary_out, metadata.binary.hash, sizeof(metadata.binary.hash));
        fd_binary_put_bytes(binary_out, metadata.binary.crypt_hash, sizeof(metadata.binary.crypt_hash));
        fd_binary_put_bytes(binary_out, metadata.binary.crypt_iv, sizeof(metadata.binary.crypt_iv));
        fd_binary_put_uint16(binary_out, metadata.revision.major);
        fd_binary_put_uint16(binary_out, metadata.revision.minor);
        fd_binary_put_uint16(binary_out, metadata.revision.patch);
        fd_binary_put_uint32(binary_out, metadata.revision.capabilities);
        fd_binary_put_bytes(binary_out, metadata.revision.commit, sizeof(metadata.revision.commit));
    }

    fd_control_send_complete(packet_output);
}

void fd_control_update_area_get_external_hash(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_get_external_hash_impl(packet_output, data, length, true);
}

void fd_control_update_area_get_sector_hashes(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_get_sector_hashes_impl(packet_output, data, length, true);
}

void fd_control_update_area_erase_sectors(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_erase_sectors_impl(packet_output, data, length, true);
}

void fd_control_update_area_write_page(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_write_page_impl(packet_output, data, length, true);
}

void fd_control_update_area_read_page(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_read_page_impl(packet_output, data, length, true);
}

void fd_control_update_area_commit(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_control_update_commit_impl(packet_output, data, length, true);
}

/////

void fd_control_radio_direct_test_mode_enter(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint16_t request = fd_binary_get_uint16(&binary);
    fd_time_t duration = fd_binary_get_time64(&binary);
    fd_bluetooth_direct_test_mode_enter(request, duration);
}

void fd_control_radio_direct_test_mode_exit(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
    fd_bluetooth_direct_test_mode_exit();
}

void fd_control_radio_direct_test_mode_report(fd_packet_output_t *packet_output, uint8_t *data __attribute__((unused)), uint32_t length __attribute__((unused))) {
    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_RADIO_DIRECT_TEST_MODE_REPORT);
    fd_binary_put_uint16(binary_out, fd_bluetooth_direct_test_mode_report());
    fd_control_send_complete(packet_output);
}

void fd_control_disconnect(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint8_t flags __attribute__((unused)) = fd_binary_get_uint8(&binary);
    // fd_bluetooth_?
    // fd_usb_?
}

static
void get_rgb(fd_binary_t *binary, fd_hal_ui_led_rgb_t *rgb) {
    rgb->r = fd_binary_get_uint8(binary);
    rgb->g = fd_binary_get_uint8(binary);
    rgb->b = fd_binary_get_uint8(binary);
}

void fd_control_led_override(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_hal_ui_led_state_t state;
    state.usb.o = fd_binary_get_uint8(&binary);
    state.usb.g = fd_binary_get_uint8(&binary);
    state.d0.r = fd_binary_get_uint8(&binary);
    get_rgb(&binary, &state.d1);
    get_rgb(&binary, &state.d2);
    get_rgb(&binary, &state.d3);
    state.d4.r = fd_binary_get_uint8(&binary);
    fd_time_t duration = fd_binary_get_time64(&binary);

    fd_hal_ui_set_led(&state, duration);
}

void fd_control_identify(fd_packet_output_t *packet_output __attribute__((unused)), uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    bool active = fd_binary_get_uint8(&binary) != 0;
    if (active) {
        fd_time_t duration = fd_binary_get_time64(&binary);
        fd_hal_ui_set_identify(duration);
    } else {
        fd_hal_ui_clear_identify();
    }
}

void fd_control_lock(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    fd_lock_identifier_t identifier = fd_binary_get_uint8(&binary);
    fd_lock_operation_t operation = fd_binary_get_uint8(&binary);
    fd_lock_owner_t owner = packet_output->owner;
    fd_lock_owner_t lock_owner = fd_lock(identifier, operation, owner);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_LOCK);
    fd_binary_put_uint8(binary_out, identifier);
    fd_binary_put_uint8(binary_out, operation);
    fd_binary_put_uint32(binary_out, lock_owner);
    fd_control_send_complete(packet_output);
}

#define FD_CONTROL_DIAGNOSTICS_FLAGS (FD_CONTROL_DIAGNOSTICS_BLE | FD_CONTROL_DIAGNOSTICS_BLE_TIMING)

void fd_control_diagnostics(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    fd_binary_t binary;
    fd_binary_initialize(&binary, data, length);
    uint32_t flags = fd_binary_get_uint32(&binary);

    fd_binary_t *binary_out = fd_control_send_start(packet_output, FD_CONTROL_DIAGNOSTICS);
    fd_binary_put_uint32(binary_out, flags & FD_CONTROL_DIAGNOSTICS_FLAGS);
    if (flags & FD_CONTROL_DIAGNOSTICS_BLE) {
        fd_bluetooth_diagnostics(binary_out);
    }
    if (flags & FD_CONTROL_DIAGNOSTICS_BLE_TIMING) {
        fd_bluetooth_diagnostics_timing(binary_out);
    }
    fd_control_send_complete(packet_output);
}

static int fd_control_ffs(int mask) {
    if (mask == 0) {
        return 0;
    }
    int bit;
    for (bit = 0; !(mask & 1); bit++) {
        mask >>= 1;
    }
    return bit;
}

void fd_control_add_property(uint32_t mask, fd_control_get_property_fn_t get, fd_control_get_property_fn_t set) {
    int i = fd_control_ffs(mask);
    fd_control_property_t *property = &fd_control_propertys[i];
    property->get = get;
    property->set = set;
}

void fd_control_initialize_commands(void) {
    fd_control_commands[FD_CONTROL_PING] = fd_control_ping;
    fd_control_commands[FD_CONTROL_GET_PROPERTIES] = fd_control_get_properties;
    fd_control_commands[FD_CONTROL_SET_PROPERTIES] = fd_control_set_properties;
    fd_control_commands[FD_CONTROL_PROVISION] = fd_control_provision;
    fd_control_commands[FD_CONTROL_RESET] = fd_control_reset;
    fd_control_commands[FD_CONTROL_RTC] = fd_control_rtc;
    fd_control_commands[FD_CONTROL_HARDWARE] = fd_control_hardware;

    fd_control_commands[FD_CONTROL_UPDATE_GET_EXTERNAL_HASH] = fd_control_update_get_external_hash;
    fd_control_commands[FD_CONTROL_UPDATE_READ_PAGE] = fd_control_update_read_page;
    fd_control_commands[FD_CONTROL_UPDATE_GET_SECTOR_HASHES] = fd_control_update_get_sector_hashes;
    fd_control_commands[FD_CONTROL_UPDATE_ERASE_SECTORS] = fd_control_update_erase_sectors;
    fd_control_commands[FD_CONTROL_UPDATE_WRITE_PAGE] = fd_control_update_write_page;
    fd_control_commands[FD_CONTROL_UPDATE_COMMIT] = fd_control_update_commit;

    fd_control_commands[FD_CONTROL_UPDATE_AREA_GET_VERSION] = fd_control_update_area_get_version;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_GET_EXTERNAL_HASH] = fd_control_update_area_get_external_hash;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_GET_SECTOR_HASHES] = fd_control_update_area_get_sector_hashes;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_ERASE_SECTORS] = fd_control_update_area_erase_sectors;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_WRITE_PAGE] = fd_control_update_area_write_page;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_READ_PAGE] = fd_control_update_area_read_page;
    fd_control_commands[FD_CONTROL_UPDATE_AREA_COMMIT] = fd_control_update_area_commit;

    fd_control_commands[FD_CONTROL_RADIO_DIRECT_TEST_MODE_ENTER] = fd_control_radio_direct_test_mode_enter;
    fd_control_commands[FD_CONTROL_RADIO_DIRECT_TEST_MODE_EXIT] = fd_control_radio_direct_test_mode_exit;
    fd_control_commands[FD_CONTROL_RADIO_DIRECT_TEST_MODE_REPORT] = fd_control_radio_direct_test_mode_report;
    fd_control_commands[FD_CONTROL_DISCONNECT] = fd_control_disconnect;
    fd_control_commands[FD_CONTROL_LED_OVERRIDE] = fd_control_led_override;
    fd_control_commands[FD_CONTROL_IDENTIFY] = fd_control_identify;
    fd_control_commands[FD_CONTROL_SYNC_START] = fd_sync_start;
    fd_control_commands[FD_CONTROL_SYNC_ACK] = fd_sync_ack;
    fd_control_commands[FD_CONTROL_LOCK] = fd_control_lock;
    fd_control_commands[FD_CONTROL_DIAGNOSTICS] = fd_control_diagnostics;
#ifndef FD_NO_SENSING
    fd_control_commands[FD_CONTROL_SENSING_SYNTHESIZE] = fd_sensing_synthesize;
#endif
    
    memset(&fd_control_propertys, 0, sizeof(fd_control_propertys));
    fd_control_add_property(FD_CONTROL_PROPERTY_VERSION, fd_control_get_property_version, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_HARDWARE_ID, fd_control_get_property_hardware_id, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_DEBUG_LOCK, fd_control_get_property_debug_lock, fd_control_set_property_debug_lock);
    fd_control_add_property(FD_CONTROL_PROPERTY_RTC, fd_control_get_property_rtc, fd_control_set_property_rtc);
    fd_control_add_property(FD_CONTROL_PROPERTY_POWER, fd_control_get_property_power, fd_control_set_property_power);
    fd_control_add_property(FD_CONTROL_PROPERTY_SITE, fd_control_get_property_site, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_RESET, fd_control_get_property_reset, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_STORAGE, fd_control_get_property_storage, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_MODE, fd_control_get_property_mode, fd_control_set_property_mode);
    fd_control_add_property(FD_CONTROL_PROPERTY_TX_POWER, fd_control_get_property_tx_power, fd_control_set_property_tx_power);
    fd_control_add_property(FD_CONTROL_PROPERTY_BOOT_VERSION, fd_control_get_property_boot_version, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_LOGGING, fd_control_get_property_logging, fd_control_set_property_logging);
    fd_control_add_property(FD_CONTROL_PROPERTY_NAME, fd_control_get_property_name, fd_control_set_property_name);
    fd_control_add_property(FD_CONTROL_PROPERTY_RETAINED, fd_control_get_property_retained, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_ADC_VDD, fd_control_get_property_adc_vdd, fd_control_set_property_adc_vdd);
    fd_control_add_property(FD_CONTROL_PROPERTY_REGULATOR, fd_control_get_property_regulator, fd_control_set_property_regulator);
#ifndef FD_NO_SENSING
    fd_control_add_property(FD_CONTROL_PROPERTY_SENSING_COUNT, fd_control_get_property_sensing_count, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_RECOGNITION, fd_control_get_property_recognition, 0);
#endif
    fd_control_add_property(FD_CONTROL_PROPERTY_INDICATE, fd_control_get_property_indicate, fd_control_set_property_indicate);
    fd_control_add_property(FD_CONTROL_PROPERTY_HARDWARE_VERSION, fd_control_get_property_hardware_version, 0);
    fd_control_add_property(FD_CONTROL_PROPERTY_SUBSCRIBE, fd_control_get_property_subscribe, fd_control_set_property_subscribe);
}

void fd_control_check_notify_properties(void) {
    fd_packet_output_t *packet_output = fd_packet_output_head;
    while (packet_output != 0) {
        uint32_t properties = packet_output->subscribed_properties & packet_output->notify_properties;
        if ((properties != 0) && packet_output->is_available()) {
            if (fd_control_send_properties(packet_output, FD_CONTROL_NOTIFY_PROPERTIES, properties)) {
                packet_output->notify_properties = 0;
            } else {
                fd_event_set_exclusive(FD_EVENT_NOTIFY);
            }
        }
        
        packet_output = packet_output->next;
    }
}

void fd_control_notify(uint32_t properties) {
    fd_packet_output_t *packet_output = fd_packet_output_head;
    while (packet_output != 0) {
        uint32_t notify_properties = packet_output->subscribed_properties & properties;
        if (notify_properties != 0) {
            packet_output->notify_properties |= notify_properties;
            fd_event_set_exclusive(FD_EVENT_NOTIFY);
        }
        
        packet_output = packet_output->next;
    }
}

// !!! should we encrypt/decrypt everything? or just syncs? or just things that modify? -denis

void fd_control_process_command(fd_packet_output_t *packet_output, uint8_t *data, uint32_t length) {
    if (length < 1) {
        return;
    }
    uint8_t code = data[0];
    fd_control_command_t command = fd_control_commands[code];
    if (command) {
        if (fd_control_before_callback) {
            fd_control_before_callback(code);
        }
        (*command)(packet_output, &data[1], length - 1);
        if (fd_control_after_callback) {
            fd_control_after_callback(code);
        }
    }
}

void fd_control_command(void) {
    int count;
    fd_packet_output_t *packet_output = 0;
    uint32_t length = 0;

    fd_hal_processor_interrupts_disable();
    count = fd_control_inputs_count;
    if (count > 0) {
        // get the command info
        fd_control_input_t *input = &fd_control_inputs[0];
        packet_output = input->packet_output;
        uint32_t input_length = input->length;
        if (input_length <= sizeof(fd_control_command_buffer)) {
            length = input_length;
            memcpy(fd_control_command_buffer, fd_control_input_buffer, input_length);
        } else {
            // to much data from the source to fit in the command buffer, so just ignore it -denis
            fd_log_assert_fail("command buffer size exceeded");
            length = 0;
        }

        // remove it from the inputs
        --fd_control_inputs_count;
        memmove(fd_control_inputs, &fd_control_inputs[1], sizeof(fd_control_input_t) * fd_control_inputs_count);
        fd_control_input_buffer_count -= input_length;
        memmove(fd_control_input_buffer, &fd_control_input_buffer[input_length], fd_control_input_buffer_count);
    }
    fd_hal_processor_interrupts_enable();

    if (count > 0) {
        // process it
        fd_control_process_command(packet_output, fd_control_command_buffer, length);
    }

    if (count > 1) {
        fd_event_set_exclusive(FD_EVENT_COMMAND);
    }
}
