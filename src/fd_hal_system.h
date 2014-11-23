#ifndef FD_HAL_SYSTEM_H
#define FD_HAL_SYSTEM_H

#define HARDWARE_ID_SIZE 16

#include "fd_range.h"

#include <stdbool.h>
#include <stdint.h>

#define FD_HAL_SYSTEM_COMMIT_SIZE 20

typedef struct {
    uint16_t major;
    uint16_t minor;
    uint16_t patch;
    uint32_t capabilities;
    uint8_t commit[FD_HAL_SYSTEM_COMMIT_SIZE];
} fd_hal_system_firmware_version_t;

typedef struct {
    uint16_t major;
    uint16_t minor;
} fd_hal_system_hardware_version_t;

void fd_hal_system_initialize(void);

fd_range_t fd_hal_system_get_firmware_update_range(uint8_t area);

void fd_hal_system_get_firmware_version(fd_hal_system_firmware_version_t *version);
void fd_hal_system_get_boot_version(fd_hal_system_firmware_version_t *version);

void fd_hal_system_get_hardware_version(fd_hal_system_hardware_version_t *version);

void fd_hal_system_set_regulator(bool switching);
bool fd_hal_system_get_regulator(void);

float fd_hal_system_get_regulated_voltage(void);

bool fd_hal_system_is_charging(void);

float fd_hal_system_get_temperature(void);
float fd_hal_system_get_battery_voltage(void);
float fd_hal_system_get_charge_current(void);

void fd_hal_system_start_conversions(void);

#endif