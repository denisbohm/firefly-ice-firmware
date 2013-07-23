#include "fd_battery.h"

double fd_battery_charge;

void fd_battery_initialize(void) {
    fd_battery_charge = 0.0;
}