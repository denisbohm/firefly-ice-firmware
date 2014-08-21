#include "fd_hal_processor.h"
#include "fd_log.h"
#include "fd_pins.h"
#include "fd_storage.h"
#include "fd_w25q16dw.h"

#include "em_gpio.h"

extern void fd_binary_unit_tests(void);
extern void fd_detour_unit_tests(void);
extern void fd_storage_unit_tests(void);
extern void fd_storage_buffer_unit_tests(void);
extern void fd_sync_unit_tests(void);

static
void chip_erase(void) {
    fd_w25q16dw_initialize();

    fd_w25q16dw_wake();
    fd_w25q16dw_enable_write();
    fd_w25q16dw_chip_erase();
    fd_w25q16dw_sleep();
}

void storage_erase(void) {
    fd_log_initialize();
    chip_erase();
    fd_storage_initialize();
}

void main(void) {
    fd_hal_processor_initialize();

    fd_binary_unit_tests();
    fd_detour_unit_tests();
    fd_storage_unit_tests();
    fd_storage_buffer_unit_tests();
    storage_erase();
    fd_sync_unit_tests();

    if (fd_log_did_log) {
        GPIO_PinOutClear(LED5_PORT_PIN);
        GPIO_PinOutSet(LED6_PORT_PIN);
    } else {
        GPIO_PinOutSet(LED5_PORT_PIN);
        GPIO_PinOutClear(LED6_PORT_PIN);
    }
}