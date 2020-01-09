#ifndef FD_POWER_H
#define FD_POWER_H

#include <stdbool.h>

typedef struct {
    float battery_level;
    float battery_voltage;
    bool is_usb_powered;
    bool is_charging;
    float charge_current;
    float temperature;
} fd_power_t;

typedef void (*fd_power_callback_t)(void);

void fd_power_initialize(void);
void fd_power_initialize_with_update_interval(uint32_t update_interval);

void fd_power_set_callback(fd_power_callback_t callback);
fd_power_callback_t fd_power_get_callback(void);

bool fd_power_is_low_battery(void);

void fd_power_set_low_battery_level_callback(fd_power_callback_t callback);
fd_power_callback_t fd_power_get_low_battery_level_callback(void);

void fd_power_set_high_battery_level_callback(fd_power_callback_t callback);
fd_power_callback_t fd_power_get_high_battery_level_callback(void);

void fd_power_get(fd_power_t *power);
void fd_power_set(float battery_level);

#endif
