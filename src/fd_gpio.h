#ifndef FD_GPIO_H
#define FD_GPIO_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint32_t port;
    uint32_t pin;
} fd_gpio_t;

typedef void (*fd_gpio_function_t)(void *context, bool pin_state);

void fd_gpio_initialize(void);

void fd_gpio_add_callback(fd_gpio_t gpio, fd_gpio_function_t function, void *context);

void fd_gpio_configure_default(fd_gpio_t gpio);

void fd_gpio_configure_output(fd_gpio_t gpio);
void fd_gpio_configure_output_open_drain(fd_gpio_t gpio);
void fd_gpio_configure_output_open_drain_pull_up(fd_gpio_t gpio);
void fd_gpio_configure_output_open_source_pull_down(fd_gpio_t gpio);

void fd_gpio_configure_input(fd_gpio_t gpio);
void fd_gpio_configure_input_pull_up(fd_gpio_t gpio);
void fd_gpio_configure_input_pull_down(fd_gpio_t gpio);

void fd_gpio_set(fd_gpio_t gpio, bool value);
bool fd_gpio_get(fd_gpio_t gpio);

#endif
