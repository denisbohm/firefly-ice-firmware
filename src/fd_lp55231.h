#ifndef FP_LP55231_H
#define FP_LP55231_H

void fd_lp55231_initialize(void);

void fd_lp55231_power_on(void);
void fd_lp55231_power_off(void);

void fd_lp55231_wake(void);
void fd_lp55231_sleep(void);

void fd_lp55231_set_led_pwm(uint8_t led, uint8_t pwm);

#endif