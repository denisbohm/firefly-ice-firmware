#ifndef FD_HAL_SYSTEM_H
#define FD_HAL_SYSTEM_H

#define HARDWARE_ID_SIZE 16

#include "fd_range.h"
#include "fd_version.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_HAL_SYSTEM_AREA_BOOTLOADER 0
#define FD_HAL_SYSTEM_AREA_APPLICATION 1
#define FD_HAL_SYSTEM_AREA_OPERATING_SYSTEM 2

void fd_hal_system_initialize(void);

bool fd_hal_system_get_crypto_key(uint8_t area, uint8_t *key);
bool fd_hal_system_get_update_metadata(uint8_t area, fd_version_metadata_t *metadata);
void fd_hal_system_set_update_metadata(uint8_t area, fd_version_metadata_t *metadata);
bool fd_hal_system_get_update_external_flash_range(uint8_t area, fd_range_t *range);
bool fd_hal_system_get_firmware_range(uint8_t area, fd_range_t *range);
bool fd_hal_system_get_firmware_version(uint8_t area, fd_version_revision_t *version);

void fd_hal_system_get_hardware_version(fd_version_hardware_t *version);

void fd_hal_system_set_regulator(bool switching);
bool fd_hal_system_get_regulator(void);

float fd_hal_system_get_regulated_voltage(void);

double fd_hal_system_get_charge_level_change_per_minute(void);
double fd_hal_system_get_discharge_level_change_per_minute(void);

bool fd_hal_system_is_charging(void);
float fd_hal_system_get_temperature(void);
float fd_hal_system_get_battery_voltage(void);
float fd_hal_system_get_charge_current(void);

void fd_hal_system_start_conversions(void);

#endif