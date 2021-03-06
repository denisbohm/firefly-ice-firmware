#include "fd_gpio.h"

#include "fd_log.h"
#include "fd_nrf5.h"

#include <string.h>

static inline
NRF_GPIO_Type *fd_gpio_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
}

#ifdef FD_GPIO_NRF52840_QFN
const static bool fd_gpio_high_drive[] = {
    true,  // P0.00
    true,  // P0.01
    false, // P0.02
    false, // P0.03
    false, // P0.04
    false, // P0.05
    false, // P0.06
    false, // P0.07
    true,  // P0.08
    false, // P0.09
    false, // P0.10
    false, // P0.11
    false, // P0.12
    true,  // P0.13
    true,  // P0.14
    true,  // P0.15
    true,  // P0.16
    true,  // P0.17
    true,  // P0.18
    true,  // P0.19
    true,  // P0.20
    true,  // P0.21
    true,  // P0.22
    true,  // P0.23
    true,  // P0.24
    true,  // P0.25
    true,  // P0.26
    true,  // P0.27
    true,  // P0.28
    true,  // P0.29
    true,  // P0.30
    true,  // P0.31

    true,  // P1.00
    false, // P1.01
    false, // P1.02
    false, // P1.03
    false, // P1.04
    false, // P1.05
    false, // P1.06
    false, // P1.07
    true,  // P1.08
    true,  // P1.09
    false, // P1.10
    false, // P1.11
    false, // P1.12
    false, // P1.13
    false, // P1.14
    false, // P1.15
};
#else
const static bool fd_gpio_high_drive[] = {};
#endif

static bool fd_gpio_is_high_drive(fd_gpio_t gpio) {
    int index = gpio.port * 32 + gpio.pin;
    if (index > sizeof(fd_gpio_high_drive)) {
        return false;
    }
    return fd_gpio_high_drive[index];
}

static
void fd_gpio_configure(
    fd_gpio_t gpio,
    nrf_gpio_pin_dir_t dir,
    nrf_gpio_pin_input_t input,
    nrf_gpio_pin_pull_t pull,
    nrf_gpio_pin_drive_t drive,
    nrf_gpio_pin_sense_t sense
) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    nrf_gpio->PIN_CNF[gpio.pin] = ((uint32_t)dir << GPIO_PIN_CNF_DIR_Pos)
                                | ((uint32_t)input << GPIO_PIN_CNF_INPUT_Pos)
                                | ((uint32_t)pull << GPIO_PIN_CNF_PULL_Pos)
                                | ((uint32_t)drive << GPIO_PIN_CNF_DRIVE_Pos)
                                | ((uint32_t)sense << GPIO_PIN_CNF_SENSE_Pos);
}

void fd_gpio_configure_default(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Input,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Disabled,
        GPIO_PIN_CNF_DRIVE_S0S1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_OUTPUT,
        NRF_GPIO_PIN_INPUT_DISCONNECT,
        NRF_GPIO_PIN_NOPULL,
        fd_gpio_is_high_drive(gpio) ? NRF_GPIO_PIN_H0H1 : NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Disabled,
        fd_gpio_is_high_drive(gpio) ? GPIO_PIN_CNF_DRIVE_H0D1 : GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pullup,
        fd_gpio_is_high_drive(gpio) ? GPIO_PIN_CNF_DRIVE_H0D1 : GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pulldown,
        fd_gpio_is_high_drive(gpio) ? GPIO_PIN_CNF_DRIVE_D0H1 : GPIO_PIN_CNF_DRIVE_D0S1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_input(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_NOPULL,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_input_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLUP,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_input_pull_down(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        NRF_GPIO_PIN_DIR_INPUT,
        NRF_GPIO_PIN_INPUT_CONNECT,
        NRF_GPIO_PIN_PULLDOWN,
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_set(fd_gpio_t gpio, bool value) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    if (value) {
        nrf_gpio->OUTSET = 1UL << gpio.pin;
    } else {
        nrf_gpio->OUTCLR = 1UL << gpio.pin;
    }
}

bool fd_gpio_get(fd_gpio_t gpio) {
    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    if (nrf_gpio->DIR & (1 << gpio.pin)) {
        return ((nrf_gpio->OUT >> gpio.pin) & 1UL) != 0;
    } else {
        return ((nrf_gpio->IN >> gpio.pin) & 1UL) != 0;
    }
}

#ifdef FD_GPIO_NRFX

#include "nrfx_gpiote.h"

typedef struct {
    fd_gpio_function_t function;
    void *context;
    uint32_t count;
} fd_gpio_nrf5_callback_t;

volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_callbacks[64];

void fd_gpio_initialize_implementation(void) {
    if (nrfx_gpiote_is_init()) {
        return;
    }
    uint32_t err_code = nrfx_gpiote_init();
    fd_log_assert(err_code == NRF_SUCCESS);
}

static void fd_gpio_nrfx_gpiote_evt_handler(nrfx_gpiote_pin_t pin, nrf_gpiote_polarity_t action) {
    volatile fd_gpio_nrf5_callback_t *callback = &fd_gpio_nrf5_callbacks[pin];
    fd_gpio_t gpio = { .port = pin / 32, .pin = pin & 0x1f };
    bool pin_state = fd_gpio_get(gpio);
    callback->function(callback->context, pin_state);
    ++callback->count;
}

void fd_gpio_add_callback(fd_gpio_t gpio, fd_gpio_edge_t edge, fd_gpio_function_t function, void *context) {
    uint32_t pin = gpio.port * 32 + gpio.pin;
    volatile fd_gpio_nrf5_callback_t *callback = &fd_gpio_nrf5_callbacks[pin];
    callback->function = function;
    callback->context = context;
    callback->count = 0;
    nrfx_gpiote_in_config_t config = {
        .hi_accuracy = true,
        .skip_gpio_setup = false,
        .is_watcher = false,
        .pull = NRF_GPIO_PIN_NOPULL,
    };
    switch (edge) {
        case fd_gpio_edge_rising:
            config.sense = NRF_GPIOTE_POLARITY_LOTOHI;
            break;
        case fd_gpio_edge_falling:
            config.sense = NRF_GPIOTE_POLARITY_HITOLO;
            break;
        case fd_gpio_edge_toggle:
        default:
            config.sense = NRF_GPIOTE_POLARITY_TOGGLE;
            break;
    }
    uint32_t err_code = nrfx_gpiote_in_init(pin, &config, fd_gpio_nrfx_gpiote_evt_handler);
    fd_log_assert(err_code == NRF_SUCCESS);
    nrfx_gpiote_in_event_enable(pin, true);
}

#else

#ifdef FD_GPIO_NRF5_PORT_EVENTS

typedef struct {
    fd_gpio_function_t function;
    void *context;
    uint32_t count;
} fd_gpio_nrf5_callback_t;

volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_p0_callbacks[32];
volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_p1_callbacks[32];

void fd_gpio_initialize_implementation(void) {
    for (int i = 0; i < 32; ++i) {
        fd_gpio_nrf5_p0_callbacks[i].function = 0;
        fd_gpio_nrf5_p0_callbacks[i].context = 0;
        fd_gpio_nrf5_p0_callbacks[i].count = 0;
        fd_gpio_nrf5_p1_callbacks[i].function = 0;
        fd_gpio_nrf5_p1_callbacks[i].context = 0;
        fd_gpio_nrf5_p1_callbacks[i].count = 0;
    }
    
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;

    NVIC_EnableIRQ(GPIOTE_IRQn);
}

void fd_gpio_add_callback(fd_gpio_t gpio, fd_gpio_function_t function, void *context) {
    volatile fd_gpio_nrf5_callback_t *callbacks = gpio.port == 0 ? fd_gpio_nrf5_p0_callbacks : fd_gpio_nrf5_p1_callbacks;
    volatile fd_gpio_nrf5_callback_t *callback = &callbacks[gpio.pin];
    callback->function = function;
    callback->context = context;

    NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;

    NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
    uint32_t sense = ((nrf_gpio->IN >> gpio.pin) & 1UL) != 0 ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High;
    volatile uint32_t *cnf = &nrf_gpio->PIN_CNF[gpio.pin];
    *cnf = (*cnf & ~GPIO_PIN_CNF_SENSE_Msk) | (sense << GPIO_PIN_CNF_SENSE_Pos);

    NRF_GPIOTE->EVENTS_PORT = 0;
    NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
}

void fd_gpio_nrf5_check(NRF_GPIO_Type *nrf_gpio, volatile fd_gpio_nrf5_callback_t *callbacks) {
    uint32_t latch = nrf_gpio->LATCH;
    for (int pin = 0; pin < 32; ++pin) {
        uint32_t mask = 1 << pin;
        if (latch & mask) {
            // clear the pin sense latch bit
            nrf_gpio->LATCH = mask;

            // toggle high vs low sense mode to get the next edge
            uint32_t cnf = nrf_gpio->PIN_CNF[pin];
            uint32_t sense = (cnf & GPIO_PIN_CNF_SENSE_Msk) >> GPIO_PIN_CNF_SENSE_Pos;
            nrf_gpio->PIN_CNF[pin] = cnf ^ (1 << GPIO_PIN_CNF_SENSE_Pos);
            
            NRF_GPIOTE->EVENTS_PORT = 0;
            (void)NRF_GPIOTE->EVENTS_PORT;

            volatile fd_gpio_nrf5_callback_t *callback = &callbacks[pin];
            callback->count += 1;
            if (callback) {
                bool pin_state = sense == GPIO_PIN_CNF_SENSE_High;
                callback->function(callback->context, pin_state);
            }
        }
    }
}

void GPIOTE_IRQHandler(void) {
    fd_gpio_nrf5_check(NRF_P0, fd_gpio_nrf5_p0_callbacks);
    fd_gpio_nrf5_check(NRF_P1, fd_gpio_nrf5_p1_callbacks);
}

#else

typedef struct {
    fd_gpio_t gpio;
    fd_gpio_edge_t edge;
    fd_gpio_function_t function;
    void *context;
    bool state;
    uint32_t count;
} fd_gpio_nrf5_callback_t;

#define fd_gpio_nrf5_callback_limit 8
volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_callbacks[fd_gpio_nrf5_callback_limit];
volatile uint32_t fd_gpio_nrf5_callback_count;

volatile bool fd_gpio_nrf5_port_used;
volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_port_callback;

void fd_gpio_initialize_implementation(void) {
    fd_gpio_nrf5_callback_count = 0;
    memset((void *)fd_gpio_nrf5_callbacks, 0, sizeof(fd_gpio_nrf5_callbacks));

    fd_gpio_nrf5_port_used = false;
    memset((void *)&fd_gpio_nrf5_port_callback, 0, sizeof(fd_gpio_nrf5_port_callback));

    NVIC_EnableIRQ(GPIOTE_IRQn);
}

void fd_gpio_add_callback(fd_gpio_t gpio, fd_gpio_edge_t edge, fd_gpio_function_t function, void *context) {
    if ((edge == fd_gpio_edge_rising) || (edge == fd_gpio_edge_falling)) {
        if (!fd_gpio_nrf5_port_used) {
            // use port change callback
            fd_gpio_nrf5_port_callback = (fd_gpio_nrf5_callback_t){
                .gpio = gpio,
                .edge = edge,
                .function = function,
                .context = context,
                .state = edge == fd_gpio_edge_rising,
                .count = 0
            };
            fd_gpio_nrf5_port_used = true;

            NRF_GPIOTE->INTENCLR = GPIOTE_INTENCLR_PORT_Msk;
            NRF_GPIO_Type *nrf_gpio = fd_gpio_get_nrf_gpio(gpio.port);
            uint32_t sense = edge == fd_gpio_edge_falling ? GPIO_PIN_CNF_SENSE_Low : GPIO_PIN_CNF_SENSE_High;
            volatile uint32_t *cnf = &nrf_gpio->PIN_CNF[gpio.pin];
            *cnf = (*cnf & ~GPIO_PIN_CNF_SENSE_Msk) | (sense << GPIO_PIN_CNF_SENSE_Pos);
            NRF_GPIOTE->EVENTS_PORT = 0;
            NRF_GPIOTE->INTENSET = GPIOTE_INTENSET_PORT_Msk;
            return;
        }
    }

    if (fd_gpio_nrf5_callback_count >= fd_gpio_nrf5_callback_limit) {
        return;
    }
    volatile fd_gpio_nrf5_callback_t *callback = &fd_gpio_nrf5_callbacks[fd_gpio_nrf5_callback_count];
    callback->gpio = gpio;
    callback->edge = edge;
    callback->function = function;
    callback->context = context;
    callback->state = fd_gpio_get(gpio);
    callback->count = 0;

    NRF_GPIOTE->CONFIG[fd_gpio_nrf5_callback_count] =
        (GPIOTE_CONFIG_MODE_Event << GPIOTE_CONFIG_MODE_Pos) |
        (gpio.pin << GPIOTE_CONFIG_PSEL_Pos) |
#ifdef GPIOTE_CONFIG_PORT_Pos
        (gpio.port << GPIOTE_CONFIG_PORT_Pos) |
#endif
        (GPIOTE_CONFIG_POLARITY_Toggle << GPIOTE_CONFIG_POLARITY_Pos);

    NRF_GPIOTE->INTENSET |= 1 << fd_gpio_nrf5_callback_count;

    fd_gpio_nrf5_callback_count += 1;
}

void GPIOTE_IRQHandler(void) {
    for (uint32_t i = 0; i < fd_gpio_nrf5_callback_limit; ++i) {
        if (NRF_GPIOTE->EVENTS_IN[i]) {
            NRF_GPIOTE->EVENTS_IN[i] = 0;
            if (i < fd_gpio_nrf5_callback_count) {
                volatile fd_gpio_nrf5_callback_t *callback = &fd_gpio_nrf5_callbacks[i];
                callback->count += 1;
                bool previous_state = callback->state;
                bool state = fd_gpio_get(callback->gpio);
                callback->state = state;
                // may have missed some transitions, make sure we at least get the same number of high & low callbacks -denis
                if (state == previous_state) {
                    callback->function(callback->context, !state);
                }
                callback->function(callback->context, state);
            }
        }
    }

    if (NRF_GPIOTE->EVENTS_PORT) {
        NRF_GPIOTE->EVENTS_PORT = 0;
        if (fd_gpio_nrf5_port_used) {
            volatile fd_gpio_nrf5_callback_t *callback = &fd_gpio_nrf5_port_callback;
            callback->count += 1;
            callback->function(callback->context, callback->state);
        }
    }
}

#endif

#endif

void fd_gpio_initialize(void) {
    fd_gpio_initialize_implementation();
}