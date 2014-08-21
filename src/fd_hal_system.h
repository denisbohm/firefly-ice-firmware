#ifndef FD_HAL_SYSTEM_H
#define FD_HAL_SYSTEM_H

#define HARDWARE_ID_SIZE 16

#include <stdbool.h>
#include <stdint.h>

uint16_t fd_hal_system_get_hardware_major(void);
uint16_t fd_hal_system_get_hardware_minor(void);

void fd_hal_system_set_regulator(bool switching);
bool fd_hal_system_get_regulator(void);

float fd_hal_system_get_regulated_voltage(void);

bool fd_hal_system_is_charging(void);

float fd_hal_system_get_temperature(void);
float fd_hal_system_get_battery_voltage(void);
float fd_hal_system_get_charge_current(void);

void fd_hal_system_start_conversions(void);

#endif