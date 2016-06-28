#include "fdi_main.h"

#include "fdi_adc.h"
#include "fdi_api.h"
#include "fdi_clock.h"
#include "fdi_gpio.h"
#include "fdi_i2c.h"
#include "fdi_led.h"
#include "fdi_mcp4726.h"
#include "fdi_relay.h"
#include "fdi_usb.h"

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

// PA3 ADC1_3 ATE_MCU1_VCC2
// PA4 ADC1_4 ATE_BS_SHR
// PA5 ADC1_5 ATE_BS_SLR
// PA6 ADC1_6 ATE_USB_CS_OUT
// PA7 ADC1_7 ATE_MCU2_VCC2
// PC5 ADC1_15 ATE_BATTERY+2

// rs = Current Sense Resistor
// ri = LTC2433 Current Sense Amplifier Rin Resistor
// ro = LTC2433 Current Sense Amplifier Rout Resistor
// rf = TLV272 Op Amp Rf Feedback Resistor
// rg = TLV272 Op Amp Rg Gain Resistor
static inline float current_sense_gain(float rs, float ri, float ro, float rf, float rg) {
    return 1.0f / (rs * (ro / ri) * ((rg + rf) / rg));
}

void fdi_main(void) {
    fdi_api_initialize();

    fdi_gpio_initialize();
    fdi_led_initialize();
    fdi_relay_initialize();

    fdi_led_on(FDI_LED_PIN_R);
    fdi_led_off(FDI_LED_PIN_R);
    fdi_led_on(FDI_LED_PIN_G);
    fdi_led_off(FDI_LED_PIN_G);
    fdi_led_on(FDI_LED_PIN_B);
    fdi_led_off(FDI_LED_PIN_B);

    fdi_gpio_on(FDI_ATE_BS_EN);

    fdi_relay_on(FDI_ATE_USB_5V_EN);
    fdi_gpio_on(FDI_ATE_USB_CS_EN);

    fdi_adc_initialize();
    fdi_adc_power_up();

    float dut_main_rail_voltage = fdi_adc_convert(3) * 2.0f;
    float dut_auxiliary_rail_voltage = fdi_adc_convert(7) * 2.0f;
    float dut_battery_voltage = fdi_adc_convert(15) * 2.0f;
    float dut_usb_current = fdi_adc_convert(6) * current_sense_gain(0.1f, 1800.0f, 12000.0f, 376.0f, 1000.0f);
    float dut_battery_current_hi = fdi_adc_convert(4) * current_sense_gain(4.7f, 1800.0f, 12000.0f, 376.0f, 1000.0f); // 0.294 A/V
    float dut_battery_current_lo = fdi_adc_convert(5) * current_sense_gain(4.7f, 4.7f, 10000.0f, 0.0f, 1000.0f); // 0.213 A/V
    float dut_battery_current = dut_battery_current_hi > 0.000250 ? dut_battery_current_hi : dut_battery_current_lo;

    fdi_i2c_initialize();
    bool did_write = fdi_mcp4726_write_volatile_dac_register(0x010d); // 4.20 V (battery charge CV)
    dut_battery_voltage = fdi_adc_convert(15) * 2.0f;
    did_write = fdi_mcp4726_write_volatile_dac_register(0x0222); // 2.75 V (battery cut-off)
    dut_battery_voltage = fdi_adc_convert(15) * 2.0f;

    fdi_clock_start_high_speed_internal();
    fdi_usb_initialize();
    fdi_api_initialize_usb();
    fdi_usb_power_up();

    while (true) {
        fdi_api_process();
    }
}
