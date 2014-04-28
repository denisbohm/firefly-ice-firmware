#ifndef FD_ADC_H
#define FD_ADC_H

#include <stdbool.h>

void fd_adc_initialize(void);

void fd_adc_set_vdd(float vdd);
float fd_adc_get_vdd(void);

void fd_adc_sleep(void);
void fd_adc_wake(void);

float fd_adc_get_temperature(void);
float fd_adc_get_battery_voltage(void);
float fd_adc_get_charge_current(void);

typedef enum {
    fd_adc_channel_temperature,
    fd_adc_channel_battery_voltage,
    fd_adc_channel_charge_current
} fd_adc_channel_t;

void fd_adc_start(fd_adc_channel_t channel, bool asynchronous);
bool fd_adc_in_progress(void);

#endif