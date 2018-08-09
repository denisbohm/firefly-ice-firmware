#include "fd_gpio.h"

#include "fd_nrf5.h"

static inline
NRF_GPIO_Type *fd_gpio_get_nrf_gpio(uint32_t port) {
    return (NRF_GPIO_Type *)(NRF_P0_BASE + port * 0x300UL);
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
        NRF_GPIO_PIN_S0S1,
        NRF_GPIO_PIN_NOSENSE
    );
}

void fd_gpio_configure_output_open_drain(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Disabled,
        GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pullup,
        GPIO_PIN_CNF_DRIVE_S0D1,
        GPIO_PIN_CNF_SENSE_Disabled
    );
}

void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio) {
    fd_gpio_configure(gpio,
        GPIO_PIN_CNF_DIR_Output,
        GPIO_PIN_CNF_INPUT_Disconnect,
        GPIO_PIN_CNF_PULL_Pulldown,
        GPIO_PIN_CNF_DRIVE_D0S1,
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
    return ((nrf_gpio->IN >> gpio.pin) & 1UL) != 0;
}

typedef struct {
    fd_gpio_function_t function;
    void *context;
    uint32_t count;
} fd_gpio_nrf5_callback_t;

volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_p0_callbacks[32];
volatile fd_gpio_nrf5_callback_t fd_gpio_nrf5_p1_callbacks[32];

void fd_gpio_initialize(void) {
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