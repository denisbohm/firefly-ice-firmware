#include "fd_quiescent_test.h"

#include "am_mcu_apollo.h"
#include "mcu.h"

#include <stdbool.h>
#include <stdint.h>

void fd_quiescent_test(void) {
    // Enable internal buck converters.
    am_hal_pwrctrl_bucks_init();
 
    // Initialize for low power in the power control block
    am_hal_pwrctrl_low_power_init();
 
    // Turn off the voltage comparator as this is enabled on reset.
    am_hal_vcomp_disable();
 
    // Run the RTC off the LFRC (so XT will not be automatically started)
    am_hal_rtc_osc_select(AM_HAL_RTC_OSC_LFRC);
 
    // Stop the XT if crystal is not populated
    am_hal_clkgen_osc_stop(AM_HAL_CLKGEN_OSC_XT);
 
    // Disable the RTC if not using
    am_hal_rtc_osc_disable();

    while (true) {
        am_hal_sysctrl_sleep(AM_HAL_SYSCTRL_SLEEP_DEEP);
        __WFI();
    }
}
