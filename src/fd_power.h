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

void fd_power_initialize(void);

void fd_power_get(fd_power_t *power);
void fd_power_set(float battery_level);

#endif
