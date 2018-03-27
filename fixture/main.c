#include "fd_i2cm.h"
#include "fd_spim.h"

static
void halt(void) {
    __asm("BKPT   #0");
}

void main(void) {
    void* tasks[] = {
        halt,

        fd_i2cm_initialize,
        fd_i2cm_bus_enable,
        fd_i2cm_bus_disable,
        fd_i2cm_bus_is_enabled,
        fd_i2cm_device_io,
        fd_i2cm_bus_wait,

        fd_spim_initialize,
        fd_spim_bus_enable,
        fd_spim_bus_disable,
        fd_spim_bus_is_enabled,
        fd_spim_device_select,
        fd_spim_device_deselect,
        fd_spim_device_is_selected,
        fd_spim_bus_io,
        fd_spim_bus_wait,
    };
}