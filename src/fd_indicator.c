#include "fd_event.h"
#include "fd_indicator.h"
#include "fd_led.h"
#include "fd_timer.h"

#include <math.h>

typedef struct fd_indicator_animation_s fd_indicator_animation_t;

typedef bool (*fd_indicator_animation_step_fn_t)(void *state);
typedef void (*fd_indicator_animation_event_fn_t)(void);

typedef struct {
    bool done;
    fd_indicator_animation_step_fn_t step_fn;
    void *step_state;
    fd_indicator_animation_event_fn_t done_fn;
} fd_indicator_animation_phase_t;

typedef struct fd_indicator_animation_s {
    bool running;
    bool cancelling;
    bool rerun;
    fd_indicator_animation_phase_t in;
    fd_indicator_animation_phase_t cycle;
    fd_indicator_animation_phase_t out;
    fd_indicator_animation_event_fn_t done_fn;
} fd_indicator_animation_t;

static uint8_t fd_indicator_animation_ease_quad_pulse[] = {
 // in
 0, 1, 2, 5, 8, 13, 19, 26,
 34, 43, 53, 64, 76, 90, 104, 119,
 136, 151, 165, 179, 191, 202, 212, 221,
 229, 236, 242, 247, 250, 253, 254, 255,
 // out
 255, 254, 253, 250, 247, 242, 236, 229,
 221, 212, 202, 191, 179, 165, 151, 136,
 119, 104, 90, 76, 64, 53, 43, 34,
 26, 19, 13, 8, 5, 2, 1, 0,
};

static uint8_t fd_indicator_animation_ease_linear_in[] = {
 0, 31, 63, 95, 127, 159, 191, 223, 255
};

static uint8_t fd_indicator_animation_ease_linear_out[] = {
 255, 223, 191, 159, 127, 95, 63, 31, 0,
};

static uint8_t fd_indicator_animation_ease_single[] = {
 255,
};

void fd_indicator_animation_phase_initialize(fd_indicator_animation_phase_t *phase) {
    phase->done = false;
    phase->step_fn = 0;
    phase->step_state = 0;
    phase->done_fn = 0;
}

void fd_indicator_animation_phase_run(fd_indicator_animation_phase_t *phase) {
    phase->done = phase->step_fn == 0;
}

bool fd_indicator_animation_phase_step(fd_indicator_animation_phase_t *phase, bool finish) {
    if (phase->done) {
        return true;
    }
    bool done = (*phase->step_fn)(phase->step_state);
    if (done) {
        if (finish) {
            phase->done = true;
        }
        if (phase->done_fn != 0) {
            phase->done_fn();
        }
    }
    return false;
}

void fd_indicator_animation_initialize(fd_indicator_animation_t *animation) {
    animation->running = false;
    animation->cancelling = false;
    animation->rerun = false;
    fd_indicator_animation_phase_initialize(&animation->in);
    fd_indicator_animation_phase_initialize(&animation->cycle);
    fd_indicator_animation_phase_initialize(&animation->out);
    animation->done_fn = 0;
}

void fd_indicator_animation_run(fd_indicator_animation_t *animation) {
    if (!animation->running) {
        animation->running = true;
        animation->cancelling = false;
        animation->rerun = false;
        fd_indicator_animation_phase_run(&animation->in);
        fd_indicator_animation_phase_run(&animation->cycle);
        fd_indicator_animation_phase_run(&animation->out);
    } else {
        if (animation->cancelling) {
            if (animation->cycle.done) {
                animation->rerun = true;
            } else {
                animation->cancelling = false;
            }
        }
    }
}

void fd_indicator_animation_cancel(fd_indicator_animation_t *animation) {
    animation->cancelling = true;
}

void fd_indicator_animation_step(fd_indicator_animation_t *animation) {
    if (animation->running) {
        if (fd_indicator_animation_phase_step(&animation->in, true)) {
            if (fd_indicator_animation_phase_step(&animation->cycle, animation->cancelling)) {
                fd_indicator_animation_phase_step(&animation->out, true);
            }
        }
        if (animation->in.done && animation->cycle.done && animation->out.done) {
            animation->running = false;
            if (animation->rerun) {
                fd_indicator_animation_run(animation);
            }
            if (!animation->running && (animation->done_fn != 0)) {
                (*animation->done_fn)();
            }
        }
    }
}

typedef struct {
    float base;
    float scale;
} fd_indicator_animation_equ_t;

uint8_t fd_indicator_animation_equ(fd_indicator_animation_equ_t equ, float v) {
    float f = equ.base + equ.scale * v;
    int32_t r = (int32_t)(f + 0.5f);
    if (r < 0) {
        return 0;
    }
    if (r > 255) {
        return 255;
    }
    return (uint8_t)r;
}

typedef struct {
    uint32_t index;
    uint32_t count;
    uint32_t size;
    uint8_t *values;
    fd_indicator_animation_equ_t r;
    fd_indicator_animation_equ_t g;
    fd_indicator_animation_equ_t b;
    uint32_t led;
} fd_indicator_animation_list_step_rgb_state_t;

void fd_indicator_animation_list_step_rgb_initialize(fd_indicator_animation_list_step_rgb_state_t *state) {
    state->index = 0;
    state->r.base = 0.0f;
    state->r.scale = 1.0f;
    state->g.base = 0.0f;
    state->g.scale = 1.0f;
    state->b.base = 0.0f;
    state->b.scale = 1.0f;
}

bool fd_indicator_animation_list_step_rgb(void *state_p) {
    fd_indicator_animation_list_step_rgb_state_t *state = state_p;
    if (state->index < state->size) {
        float v = state->values[state->index];
        uint8_t r = fd_indicator_animation_equ(state->r, v);
        uint8_t g = fd_indicator_animation_equ(state->g, v);
        uint8_t b = fd_indicator_animation_equ(state->b, v);
        switch (state->led) {
            case 1:
                fd_led_set_d1(r, g, b);
                break;
            case 2:
                fd_led_set_d2(r, g, b);
                break;
            case 3:
                fd_led_set_d3(r, g, b);
                break;
            default:
                break;
        }
    }
    ++state->index;
    bool done = state->index >= state->size;
    if (done) {
        if (state->index >= state->count) {
            state->index = 0;
        }
    }
    return done;
}

typedef struct {
    uint32_t index;
    uint32_t size;
    uint8_t *values;
    fd_indicator_animation_equ_t o;
    fd_indicator_animation_equ_t g;
} fd_indicator_animation_list_step_og_state_t;

void fd_indicator_animation_list_step_og_initialize(fd_indicator_animation_list_step_og_state_t *state) {
    state->index = 0;
    state->o.base = 0.0f;
    state->o.scale = 1.0f;
    state->g.base = 0.0f;
    state->g.scale = 1.0f;
}

bool fd_indicator_animation_list_step_og(void *state_p) {
    fd_indicator_animation_list_step_og_state_t *state = state_p;
    float v = state->values[state->index++];
    uint8_t o = fd_indicator_animation_equ(state->o, v);
    uint8_t g = fd_indicator_animation_equ(state->g, v);
    fd_led_set_usb(o, g);
    bool done = state->index >= state->size;
    if (done) {
        state->index = 0;
    }
    return done;
}

typedef struct {
    uint32_t index;
    uint32_t size;
    uint8_t *values;
    fd_indicator_animation_equ_t r;
    uint32_t led;
} fd_indicator_animation_list_step_r_state_t;

void fd_indicator_animation_list_step_r_initialize(fd_indicator_animation_list_step_r_state_t *state) {
    state->index = 0;
    state->r.base = 0.0f;
    state->r.scale = 1.0f;
}

bool fd_indicator_animation_list_step_r(void *state_p) {
    fd_indicator_animation_list_step_r_state_t *state = state_p;
    float v = state->values[state->index++];
    uint8_t r = fd_indicator_animation_equ(state->r, v);
    switch (state->led) {
        case 0:
            fd_led_set_d0(r);
            break;
        case 4:
            fd_led_set_d4(r);
            break;
        default:
            break;
    }
    bool done = state->index >= state->size;
    if (done) {
        state->index = 0;
    }
    return done;
}

// USB power indication

typedef struct {
    fd_indicator_usb_condition_t condition_showing;
    fd_indicator_usb_condition_t condition_current;

    fd_indicator_animation_list_step_og_state_t in_state;
    fd_indicator_animation_list_step_og_state_t cycle_state;
    fd_indicator_animation_list_step_og_state_t out_state;
    fd_indicator_animation_t animation;
} fd_indicator_usb_t;

static
fd_indicator_usb_t usb;

static
void usb_initialize(void) {
    usb.condition_showing = usb.condition_current = fd_indicator_usb_condition_unpowered;
    fd_indicator_animation_initialize(&usb.animation);
}

static
void usb_off(void) {
    fd_led_set_usb(0, 0);
}

static
void usb_show(void);

static
void usb_run(
    float o, float g,
    float in_base, float in_scale,
    float cycle_base, float cycle_scale,
    float out_base, float out_scale
) {
    usb.in_state.o.base = o * in_base;
    usb.in_state.o.scale = o * in_scale;
    usb.in_state.g.base = g * in_base;
    usb.in_state.g.scale = g * in_scale;

    usb.cycle_state.o.base = o * cycle_base;
    usb.cycle_state.o.scale = o * cycle_scale;
    usb.cycle_state.g.base = g * cycle_base;
    usb.cycle_state.g.scale = g * cycle_scale;

    usb.out_state.o.base = o * out_base;
    usb.out_state.o.scale = o * out_scale;
    usb.out_state.g.base = g * out_base;
    usb.out_state.g.scale = g * out_scale;

    fd_indicator_animation_initialize(&usb.animation);
    usb.animation.in.step_fn = fd_indicator_animation_list_step_og;
    usb.animation.in.step_state = &usb.in_state;
    usb.animation.cycle.step_fn = fd_indicator_animation_list_step_og;
    usb.animation.cycle.step_state = &usb.cycle_state;
    usb.animation.out.step_fn = fd_indicator_animation_list_step_og;
    usb.animation.out.step_state = &usb.out_state;
    usb.animation.done_fn = usb_show;

    fd_indicator_animation_run(&usb.animation);
}

static
void usb_start_animation_powered_not_charging(void) {
    fd_indicator_animation_list_step_og_initialize(&usb.in_state);
    usb.in_state.size = sizeof(fd_indicator_animation_ease_linear_in);
    usb.in_state.values = fd_indicator_animation_ease_linear_in;

    fd_indicator_animation_list_step_og_initialize(&usb.cycle_state);
    usb.cycle_state.size = sizeof(fd_indicator_animation_ease_single);
    usb.cycle_state.values = fd_indicator_animation_ease_single;

    fd_indicator_animation_list_step_og_initialize(&usb.out_state);
    usb.out_state.size = sizeof(fd_indicator_animation_ease_linear_out);
    usb.out_state.values = fd_indicator_animation_ease_linear_out;

    usb_run(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

static
void usb_start_animation_powered_charging(void) {
    fd_indicator_animation_list_step_og_initialize(&usb.in_state);
    usb.in_state.size = sizeof(fd_indicator_animation_ease_linear_in);
    usb.in_state.values = fd_indicator_animation_ease_linear_in;

    fd_indicator_animation_list_step_og_initialize(&usb.cycle_state);
    usb.cycle_state.size = sizeof(fd_indicator_animation_ease_quad_pulse);
    usb.cycle_state.values = fd_indicator_animation_ease_quad_pulse;

    fd_indicator_animation_list_step_og_initialize(&usb.out_state);
    usb.out_state.size = sizeof(fd_indicator_animation_ease_linear_out);
    usb.out_state.values = fd_indicator_animation_ease_linear_out;

    usb_run(1.0f, 1.0f, 0.0f, 0.2f, 51.0f, 0.8f, 0.0f, 0.2f);
}

static
void usb_show(void) {
    switch (usb.condition_current) {
        case fd_indicator_usb_condition_unpowered:
            usb_off();
        break;
        case fd_indicator_usb_condition_powered_not_charging:
            usb_start_animation_powered_not_charging();
        break;
        case fd_indicator_usb_condition_powered_charging:
            usb_start_animation_powered_charging();
        break;
    }
    usb.condition_showing = usb.condition_current;
}

void fd_indicator_set_usb_condition(fd_indicator_usb_condition_t condition) {
    if (usb.condition_current == condition) {
        return;
    }

    if (usb.condition_showing == condition) {
        fd_indicator_animation_run(&usb.animation);
        usb.condition_current = condition;
        return;
    }

    if (usb.animation.running) {
        usb.animation.cancelling = true;
        usb.condition_current = condition;
        return;
    }

    // start usb animation
    usb.condition_current = condition;

    usb_show();
}

// communication indication

typedef struct {
    fd_indicator_connection_condition_t condition_showing;
    fd_indicator_connection_condition_t condition_current;

    fd_indicator_animation_list_step_rgb_state_t cycle_state;
    fd_indicator_animation_t animation;

    uint32_t led;
    float r;
    float g;
    float b;
    fd_indicator_animation_event_fn_t done_fn;
} fd_indicator_connection_t;

static
void connection_initialize(fd_indicator_connection_t *connection, uint32_t led, float r, float g, float b, fd_indicator_animation_event_fn_t done_fn) {
    connection->condition_showing = connection->condition_current = fd_indicator_connection_condition_unconnected;
    fd_indicator_animation_initialize(&connection->animation);
    connection->led = led;
    connection->r = r;
    connection->g = g;
    connection->b = b;
    connection->done_fn = done_fn;
}

static
void connection_off(fd_indicator_connection_t *connection) {
    switch (connection->led) {
        case 1:
            fd_led_set_d1(0, 0, 0);
        break;
        case 2:
            fd_led_set_d2(0, 0, 0);
        break;
        case 3:
            fd_led_set_d3(0, 0, 0);
        break;
    }
}

static
void connection_run(
    fd_indicator_connection_t *connection,
    float cycle_base, float cycle_scale
) {
    connection->cycle_state.r.base = connection->r * cycle_base;
    connection->cycle_state.r.scale = connection->r * cycle_scale;
    connection->cycle_state.g.base = connection->g * cycle_base;
    connection->cycle_state.g.scale = connection->g * cycle_scale;
    connection->cycle_state.b.base = connection->b * cycle_base;
    connection->cycle_state.b.scale = connection->b * cycle_scale;
    connection->cycle_state.led = connection->led;

    fd_indicator_animation_t *animation = &connection->animation;
    fd_indicator_animation_initialize(animation);
    animation->cycle.step_fn = fd_indicator_animation_list_step_rgb;
    animation->cycle.step_state = &connection->cycle_state;
    animation->done_fn = connection->done_fn;

    fd_indicator_animation_run(animation);
}

static
void connection_start_animation_not_syncing(fd_indicator_connection_t *connection) {
    fd_indicator_animation_list_step_rgb_state_t *cycle_state = &connection->cycle_state;
    fd_indicator_animation_list_step_rgb_initialize(cycle_state);
    cycle_state->count = 3 * 32 + sizeof(fd_indicator_animation_ease_quad_pulse);
    cycle_state->size = sizeof(fd_indicator_animation_ease_quad_pulse);
    cycle_state->values = fd_indicator_animation_ease_quad_pulse;

    connection_run(connection, 0.0f, 0.1f);
}

static
void connection_start_animation_syncing(fd_indicator_connection_t *connection) {
    fd_indicator_animation_list_step_rgb_state_t *cycle_state = &connection->cycle_state;
    fd_indicator_animation_list_step_rgb_initialize(cycle_state);
    cycle_state->count = sizeof(fd_indicator_animation_ease_quad_pulse);
    cycle_state->size = sizeof(fd_indicator_animation_ease_quad_pulse);
    cycle_state->values = fd_indicator_animation_ease_quad_pulse;

    connection_run(connection, 0.0f, 1.0f);
}

static
void connection_show(fd_indicator_connection_t *connection) {
    switch (connection->condition_current) {
        case fd_indicator_connection_condition_unconnected:
            connection_off(connection);
        break;
        case fd_indicator_connection_condition_not_syncing:
            connection_start_animation_not_syncing(connection);
        break;
        case fd_indicator_connection_condition_syncing:
            connection_start_animation_syncing(connection);
        break;
    }
    connection->condition_showing = connection->condition_current;
}

static
void connection_set_condition(fd_indicator_connection_t *connection, fd_indicator_connection_condition_t condition) {
    if (connection->condition_current == condition) {
        return;
    }

    if (connection->condition_showing == condition) {
        fd_indicator_animation_run(&connection->animation);
        connection->condition_current = condition;
        return;
    }

    if (connection->animation.running) {
        connection->animation.cancelling = true;
        connection->condition_current = condition;
        return;
    }

    // start usb animation
    connection->condition_current = condition;

    connection_show(connection);
}

// usb communication indication

static
fd_indicator_connection_t usb_connection;

static
void usb_connection_show(void) {
    connection_show(&usb_connection);
}

void fd_indicator_set_usb_connection_condition(fd_indicator_connection_condition_t condition) {
    connection_set_condition(&usb_connection, condition);
}

// ble communication indication

static
fd_indicator_connection_t ble_connection;

static
void ble_connection_show(void) {
    connection_show(&ble_connection);
}

void fd_indicator_set_ble_connection_condition(fd_indicator_connection_condition_t condition) {
    connection_set_condition(&ble_connection, condition);
}

// identify indication

typedef struct {
    fd_indicator_identify_condition_t condition_showing;
    fd_indicator_identify_condition_t condition_current;

    fd_indicator_animation_list_step_rgb_state_t cycle_state;
    fd_indicator_animation_t animation;
} fd_indicator_identify_t;

static
fd_indicator_identify_t identify;

static
fd_timer_t identify_timer;

static
void identify_timer_callback(void) {
    fd_indicator_set_identify_condition(fd_indicator_identify_condition_inactive);
}

static
void identify_initialize(void) {
    identify.condition_showing = identify.condition_current = fd_indicator_identify_condition_inactive;
    fd_indicator_animation_initialize(&identify.animation);
    identify.cycle_state.led = 2;

    fd_timer_add(&identify_timer, identify_timer_callback);
}

static
void identify_off(void) {
    fd_led_set_d2(0, 0, 0);
}

static
void identify_show(void);

static
void identify_run(
    float r, float g, float b,
    float cycle_base, float cycle_scale
) {
    identify.cycle_state.r.base = r * cycle_base;
    identify.cycle_state.r.scale = r * cycle_scale;
    identify.cycle_state.g.base = g * cycle_base;
    identify.cycle_state.g.scale = g * cycle_scale;
    identify.cycle_state.b.base = b * cycle_base;
    identify.cycle_state.b.scale = b * cycle_scale;

    fd_indicator_animation_initialize(&identify.animation);
    identify.animation.cycle.step_fn = fd_indicator_animation_list_step_rgb;
    identify.animation.cycle.step_state = &identify.cycle_state;
    identify.animation.done_fn = identify_show;

    fd_indicator_animation_run(&identify.animation);
}

static
void identify_start_animation_active(void) {
    fd_indicator_animation_list_step_rgb_initialize(&identify.cycle_state);
    identify.cycle_state.count = sizeof(fd_indicator_animation_ease_quad_pulse);
    identify.cycle_state.size = sizeof(fd_indicator_animation_ease_quad_pulse);
    identify.cycle_state.values = fd_indicator_animation_ease_quad_pulse;

    identify_run(1.0f, 1.0f, 1.0f, 0.0f, 1.0f);
}

static
void identify_show(void) {
    switch (identify.condition_current) {
        case fd_indicator_identify_condition_inactive:
            identify_off();
        break;
        case fd_indicator_identify_condition_active:
            identify_start_animation_active();
        break;
    }
    identify.condition_showing = identify.condition_current;
}

void fd_indicator_set_identify_condition(fd_indicator_identify_condition_t condition) {
    if (identify.condition_current == condition) {
        return;
    }

    if (identify.condition_showing == condition) {
        fd_indicator_animation_run(&identify.animation);
        identify.condition_current = condition;
        return;
    }

    if (identify.animation.running) {
        identify.animation.cancelling = true;
        identify.condition_current = condition;
        return;
    }

    // start identify animation
    identify.condition_current = condition;

    identify_show();
}

void fd_indicator_set_identify_condition_active(fd_time_t duration) {
    fd_indicator_set_identify_condition(fd_indicator_identify_condition_active);
    fd_timer_start(&identify_timer, duration);
}

// error indication

typedef struct {
    fd_indicator_error_condition_t condition_showing;
    fd_indicator_error_condition_t condition_current;

    fd_indicator_animation_list_step_r_state_t cycle_state;
    fd_indicator_animation_t animation;
} fd_indicator_error_t;

static
fd_indicator_error_t error;

static
void error_initialize(void) {
    error.condition_showing = error.condition_current = fd_indicator_error_condition_inactive;
    fd_indicator_animation_initialize(&error.animation);
}

static
void error_off(void) {
    fd_led_set_d0(0);
    fd_led_set_d4(0);
}

static
void error_flip(void) {
    error.cycle_state.led = error.cycle_state.led == 0 ? 4 : 0;
}

static
void error_show(void);

static
void error_start_animation_active(void) {
    fd_indicator_animation_list_step_r_initialize(&error.cycle_state);
    error.cycle_state.size = sizeof(fd_indicator_animation_ease_quad_pulse);
    error.cycle_state.values = fd_indicator_animation_ease_quad_pulse;
    error.cycle_state.led = 0;

    fd_indicator_animation_initialize(&error.animation);
    error.animation.cycle.step_fn = fd_indicator_animation_list_step_r;
    error.animation.cycle.step_state = &error.cycle_state;
    error.animation.cycle.done_fn = error_flip;
    error.animation.done_fn = error_show;

    fd_indicator_animation_run(&error.animation);
}

static
void error_show(void) {
    switch (error.condition_current) {
        case fd_indicator_error_condition_inactive:
            error_off();
        break;
        case fd_indicator_error_condition_active:
            error_start_animation_active();
        break;
    }
    error.condition_showing = error.condition_current;
}

void fd_indicator_set_error_condition(fd_indicator_error_condition_t condition) {
    if (error.condition_current == condition) {
        return;
    }

    if (error.condition_showing == condition) {
        fd_indicator_animation_run(&error.animation);
        error.condition_current = condition;
        return;
    }

    if (error.animation.running) {
        error.animation.cancelling = true;
        error.condition_current = condition;
        return;
    }

    // start error animation
    error.condition_current = condition;

    error_show();
}

// indicator

void fd_indicator_step(void) {
    fd_indicator_animation_step(&usb.animation);
    fd_indicator_animation_step(&usb_connection.animation);
    fd_indicator_animation_step(&ble_connection.animation);
    fd_indicator_animation_step(&identify.animation);
    fd_indicator_animation_step(&error.animation);
}

void fd_indicator_sleep(void) {
    usb_initialize();
    connection_initialize(&usb_connection, 3, 0.0f, 1.0f, 0.0f, usb_connection_show);
    connection_initialize(&ble_connection, 1, 0.0f, 0.0f, 1.0f, ble_connection_show);
    identify_initialize();
    error_initialize();
}

void fd_indicator_initialize(void) {
    fd_indicator_sleep();

    fd_event_add_callback(FD_EVENT_RTC_TICK, fd_indicator_step);
}
