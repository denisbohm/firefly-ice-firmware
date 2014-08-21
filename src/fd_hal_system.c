#include "fd_adc.h"
#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_system.h"
#include "fd_pins.h"

#include <em_gpio.h>

#define HARDWARE_MAJOR 1
#define HARDWARE_MINOR 3

uint16_t fd_hal_system_get_hardware_major(void) {
    return HARDWARE_MAJOR;
}

uint16_t fd_hal_system_get_hardware_minor(void) {
    return HARDWARE_MINOR;
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

void fd_hal_system_init(void) {
    fd_event_add_callback(FD_EVENT_ADC_TEMPERATURE, fd_hal_system_temperature_callback);
    fd_event_add_callback(FD_EVENT_ADC_BATTERY_VOLTAGE, fd_hal_system_battery_voltage_callback);
    fd_event_add_callback(FD_EVENT_ADC_CHARGE_CURRENT, fd_hal_system_charge_current_callback);
}