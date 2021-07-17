#ifndef FDI_GPIO_INSTRUMENT_H
#define FDI_GPIO_INSTRUMENT_H

#include "fdi_gpio.h"
#include "fdi_instrument.h"

typedef enum {
    fdi_gpio_instrument_domain_digital = 0,
    fdi_gpio_instrument_domain_analog = 1,
} fdi_gpio_instrument_domain_t;

typedef enum {
    fdi_gpio_instrument_direction_input = 0,
    fdi_gpio_instrument_direction_output = 1,
} fdi_gpio_instrument_direction_t;

typedef enum {
    fdi_gpio_instrument_drive_push_pull = 0,
    fdi_gpio_instrument_drive_open_drain = 1,
} fdi_gpio_instrument_drive_t;

typedef enum {
    fdi_gpio_instrument_pull_none = 0,
    fdi_gpio_instrument_pull_up = 1,
    fdi_gpio_instrument_pull_down = 2,
} fdi_gpio_instrument_pull_t;

typedef struct {
    fdi_gpio_instrument_domain_t domain;
    fdi_gpio_instrument_direction_t direction;
    fdi_gpio_instrument_drive_t drive;
    fdi_gpio_instrument_pull_t pull;
} fdi_gpio_instrument_configuration_t;

typedef struct {
    uint32_t gpio;
    bool has_adc;
    uint32_t adc_channel;
    bool has_dac;
    uint32_t dac_channel;
    bool has_auxiliary;
    uint32_t auxiliary_gpio;
} fdi_gpio_instrument_setup_t;

typedef struct {
    fdi_instrument_t super;
    fdi_gpio_instrument_configuration_t configuration;
    fdi_gpio_instrument_configuration_t auxiliary_configuration;
    const fdi_gpio_instrument_setup_t *setup;
} fdi_gpio_instrument_t;

void fdi_gpio_instrument_initialize(void);

uint32_t fdi_gpio_instrument_get_count(void);
fdi_gpio_instrument_t *fdi_gpio_instrument_get_at(uint32_t index);

void fdi_gpio_instrument_reset(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_configuration(fdi_gpio_instrument_t *instrument, const fdi_gpio_instrument_configuration_t *configuration);
bool fdi_gpio_instrument_get_digital_input(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_digital_output(fdi_gpio_instrument_t *instrument, bool bit);
float fdi_gpio_instrument_get_analog_input(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_analog_output(fdi_gpio_instrument_t *instrument, float voltage);
void fdi_gpio_instrument_set_auxiliary_configuration(fdi_gpio_instrument_t *instrument, const fdi_gpio_instrument_configuration_t *configuration);
bool fdi_gpio_instrument_get_auxiliary_input(fdi_gpio_instrument_t *instrument);
void fdi_gpio_instrument_set_auxiliary_output(fdi_gpio_instrument_t *instrument, bool bit);

#endif
