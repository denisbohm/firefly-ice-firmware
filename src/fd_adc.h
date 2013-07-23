#ifndef FD_ADC_H
#define FD_ADC_H

#include <stdbool.h>

void fd_adc_initialize(void);

void fd_adc_sleep(void);
void fd_adc_wake(void);

bool fd_adc_in_progress(void);
void fd_adc_start(void);

#endif