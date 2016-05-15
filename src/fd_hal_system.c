#include "fd_adc.h"
#include "fd_boot.h"
#include "fd_control_codes.h"
#include "fd_event.h"
#include "fd_hal_aes.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_pins.h"

#include <em_gpio.h>

#include <string.h>

#define HARDWARE_MAJOR 1
#define HARDWARE_MINOR 3

#define FIRMWARE_MAJOR 1
#define FIRMWARE_MINOR 0
#define FIRMWARE_PATCH 50

#define FIRMWARE_CAPABILITIES (\
 FD_CONTROL_CAPABILITY_LOCK |\
 FD_CONTROL_CAPABILITY_BOOT_VERSION |\
 FD_CONTROL_CAPABILITY_SYNC_AHEAD |\
 FD_CONTROL_CAPABILITY_IDENTIFY |\
 FD_CONTROL_CAPABILITY_LOGGING |\
 FD_CONTROL_CAPABILITY_DIAGNOSTICS |\
 FD_CONTROL_CAPABILITY_NAME |\
 FD_CONTROL_CAPABILITY_RETAINED |\
 FD_CONTROL_CAPABILITY_ADC_VDD |\
 FD_CONTROL_CAPABILITY_REGULATOR |\
 FD_CONTROL_CAPABILITY_SENSING_COUNT |\
 FD_CONTROL_CAPABILITY_INDICATE |\
 FD_CONTROL_CAPABILITY_RECOGNITION |\
 FD_CONTROL_CAPABILITY_HARDWARE_VERSION |\
 FD_CONTROL_CAPABILITY_RTC |\
 FD_CONTROL_CAPABILITY_HARDWARE)

// should come from gcc command line define for release build -denis
#ifndef FIRMWARE_COMMIT
#define FIRMWARE_COMMIT 0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19
#endif

static const uint8_t fd_hal_system_firmware_commit[] = {FIRMWARE_COMMIT};

bool fd_hal_system_get_update_external_flash_range(uint8_t area, fd_range_t *range) {
    switch (area) {
        case FD_HAL_SYSTEM_AREA_APPLICATION:
            *range = fd_range_make(0, 0x40000);
            return true;
        default:
            break;
    }
    return false;
}

bool fd_hal_system_get_firmware_version(uint8_t area, fd_version_revision_t *version) {
    switch (area) {
        case FD_HAL_SYSTEM_AREA_BOOTLOADER:
            version->major = FIRMWARE_MAJOR;
            version->minor = FIRMWARE_MINOR;
            version->patch = FIRMWARE_PATCH;
            version->capabilities = FIRMWARE_CAPABILITIES;
            memcpy(version->commit, fd_hal_system_firmware_commit, FD_VERSION_COMMIT_SIZE);
            return true;
        default:
            break;
    }
    return false;
}

static
fd_boot_data_t *fd_hal_system_get_boot_data_address(void) {
     return (fd_boot_data_t *)0x6f00;
}

void fd_hal_system_get_boot_version(fd_version_revision_t *version) {
    fd_boot_data_t boot_data = *fd_hal_system_get_boot_data_address();
    if (boot_data.magic != FD_BOOT_MAGIC) {
        memset(&boot_data, 0, sizeof(fd_boot_data_t));
        boot_data.minor = 1;
    }
    version->major = boot_data.major;
    version->minor = boot_data.minor;
    version->patch = boot_data.patch;
    version->capabilities = boot_data.capabilities;
    memcpy(version->commit, boot_data.git_commit, FD_VERSION_COMMIT_SIZE);
}

void fd_hal_system_get_hardware_version(fd_version_hardware_t *version) {
    version->major = HARDWARE_MAJOR;
    version->minor = HARDWARE_MINOR;
}

bool fd_hal_system_get_firmware_range(uint8_t area, fd_range_t *range) {
    switch (area) {
        case FD_HAL_SYSTEM_AREA_BOOTLOADER:
            *range = fd_range_make(0x00000000, 0x00007000);
            return true;
        case FD_HAL_SYSTEM_AREA_APPLICATION:
            *range = fd_range_make(0x00008000, 0x00040000 - 0x00008000);
            return true;
        default:
            break;
    }
    return false;
}

bool fd_hal_system_get_crypto_key(uint8_t area __attribute__((unused)), uint8_t *key) {
    fd_range_t range = fd_range_make(0x00007000, 0x00000800);
    memcpy(key, (uint8_t *)range.address, FD_HAL_AES_KEY_SIZE);
    return true;
}

static
fd_range_t fd_hal_system_get_firmware_update_metadata_range(uint8_t area __attribute__((unused))) {
    return fd_range_make(0x00007800, 0x00000800);
}

bool fd_hal_system_get_update_metadata(uint8_t area, fd_version_metadata_t *metadata) {
    fd_version_metadata_stored_t *metadata_stored = (fd_version_metadata_stored_t *)fd_hal_system_get_firmware_update_metadata_range(area).address;
    *metadata = metadata_stored->metadata;
    return metadata_stored->magic == FD_VERSION_MAGIC;
}

void fd_hal_system_set_update_metadata(uint8_t area, fd_version_metadata_t *metadata) {
    fd_version_metadata_stored_t metadata_stored;
    metadata_stored.magic = FD_VERSION_MAGIC;
    metadata_stored.metadata = *metadata;
    fd_hal_processor_write_flash_data((void *)fd_hal_system_get_firmware_update_metadata_range(area).address, (uint8_t *)&metadata_stored, sizeof(fd_version_metadata_stored_t));
}

void fd_hal_system_set_regulator(bool switching) {
    if (switching) {
        GPIO_PinModeSet(PWR_MODE_PORT_PIN, gpioModePushPull, 0);
        GPIO_PinModeSet(PWR_HIGH_PORT_PIN, gpioModePushPull, 1);
        fd_hal_processor_delay_ms(1);
        GPIO_PinModeSet(PWR_SEL_PORT_PIN, gpioModePushPull, 1);
    } else {
        GPIO_PinModeSet(PWR_SEL_PORT_PIN, gpioModePushPull, 0);
        GPIO_PinModeSet(PWR_MODE_PORT_PIN, gpioModePushPull, 0);
        GPIO_PinModeSet(PWR_HIGH_PORT_PIN, gpioModePushPull, 0);
    }
}

bool fd_hal_system_get_regulator(void) {
    return GPIO_PinInGet(PWR_SEL_PORT_PIN) != 0;
}

float fd_hal_system_get_regulated_voltage(void) {
    return 2.5f;
}

float fd_hal_system_get_charging_low_battery_voltage(void) {
    return 3.7f;
}

float fd_hal_system_get_almost_low_battery_voltage(void) {
    return 3.625f;
}

float fd_hal_system_get_low_battery_voltage(void) {
    return 3.6f;
}

double fd_hal_system_get_charge_current_limit(void) {
    return 0.08;
}

double fd_hal_system_get_charge_current_limit_threshold(void) {
    return fd_hal_system_get_charge_current_limit() * 0.9;
}

double fd_hal_system_get_battery_level_at_cccv_threshold(void) {
    return 0.85;
}

double fd_hal_system_get_discharge_threshold_voltage(void) {
    return 4.0;
}

double fd_hal_system_get_discharge_threshold_level(void) {
    return 0.9;
}

double fd_hal_system_get_recharge_threshold_voltage(void) {
    return 4.05;
}

double fd_hal_system_get_charge_level_change_per_minute(void) {
    // full charge over 2 hours
    return 1.0 / (2.0 * 60.0);
}

double fd_hal_system_get_discharge_level_change_per_minute(void) {
    // full discharge over 2 weeks
    return 1.0 / (14.0 * 24.0 * 60.0);
}

bool fd_hal_system_is_charging(void) {
    return GPIO_PinInGet(CHG_STAT_PORT_PIN) == 0;
}

float fd_hal_system_get_temperature(void) {
    return fd_adc_get_temperature();
}

float fd_hal_system_get_battery_voltage(void) {
    return fd_adc_get_battery_voltage();
}

float fd_hal_system_get_charge_current(void) {
    return fd_adc_get_charge_current();
}

void fd_hal_system_start_conversions(void) {
    fd_adc_start(fd_adc_channel_temperature, true);
}

void fd_hal_system_temperature_callback(void) {
    fd_adc_start(fd_adc_channel_battery_voltage, true);
}

void fd_hal_system_battery_voltage_callback(void) {
    fd_adc_start(fd_adc_channel_charge_current, true);
}

void fd_hal_system_charge_current_callback(void) {
    // all conversions complete
}

void fd_hal_system_initialize(void) {
    fd_event_add_callback(FD_EVENT_ADC_TEMPERATURE, fd_hal_system_temperature_callback);
    fd_event_add_callback(FD_EVENT_ADC_BATTERY_VOLTAGE, fd_hal_system_battery_voltage_callback);
    fd_event_add_callback(FD_EVENT_ADC_CHARGE_CURRENT, fd_hal_system_charge_current_callback);
}
