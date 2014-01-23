#include "fd_event.h"
#include "fd_indicator.h"
#include "fd_led.h"

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
    if (finish && done) {
        phase->done = true;
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
    uint32_t size;
    uint8_t *values;
    fd_indicator_animation_equ_t r;
    fd_indicator_animation_equ_t g;
    fd_indicator_animation_equ_t b;
    uint32_t led;
} fd_indicator_animation_list_step_rgb_state_t;

bool fd_indicator_animation_list_step_rgb(void *state_p) {
    fd_indicator_animation_list_step_rgb_state_t *state = state_p;
    float v = state->values[state->index++];
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
    return state->index >= state->size;
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
    return state->index >= state->size;
}

// When the condition changes the previous condition animation should complete its cycle
// before the new condition animation starts.

static
fd_indicator_usb_condition_t usb_condition_showing;
static
fd_indicator_usb_condition_t usb_condition_current;

static
fd_indicator_animation_list_step_og_state_t usb_in_og_state;
static
fd_indicator_animation_list_step_og_state_t usb_cycle_og_state;
static
fd_indicator_animation_list_step_og_state_t usb_out_og_state;
static
fd_indicator_animation_t usb_animation;

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
    usb_in_og_state.o.base = o * in_base;
    usb_in_og_state.o.scale = o * in_scale;
    usb_in_og_state.g.base = g * in_base;
    usb_in_og_state.g.scale = g * in_scale;

    usb_cycle_og_state.o.base = o * cycle_base;
    usb_cycle_og_state.o.scale = o * cycle_scale;
    usb_cycle_og_state.g.base = g * cycle_base;
    usb_cycle_og_state.g.scale = g * cycle_scale;

    usb_out_og_state.o.base = o * out_base;
    usb_out_og_state.o.scale = o * out_scale;
    usb_out_og_state.g.base = g * out_base;
    usb_out_og_state.g.scale = g * out_scale;

    fd_indicator_animation_initialize(&usb_animation);
    usb_animation.in.step_fn = fd_indicator_animation_list_step_og;
    usb_animation.in.step_state = &usb_in_og_state;
    usb_animation.cycle.step_fn = fd_indicator_animation_list_step_og;
    usb_animation.cycle.step_state = &usb_cycle_og_state;
    usb_animation.out.step_fn = fd_indicator_animation_list_step_og;
    usb_animation.out.step_state = &usb_out_og_state;
    usb_animation.done_fn = usb_show;

    fd_indicator_animation_run(&usb_animation);
}

static
void usb_start_animation_powered_not_charging(void) {
    fd_indicator_animation_list_step_og_initialize(&usb_in_og_state);
    usb_in_og_state.size = sizeof(fd_indicator_animation_ease_linear_in);
    usb_in_og_state.values = fd_indicator_animation_ease_linear_in;

    fd_indicator_animation_list_step_og_initialize(&usb_cycle_og_state);
    usb_cycle_og_state.size = sizeof(fd_indicator_animation_ease_single);
    usb_cycle_og_state.values = fd_indicator_animation_ease_single;

    fd_indicator_animation_list_step_og_initialize(&usb_out_og_state);
    usb_out_og_state.size = sizeof(fd_indicator_animation_ease_linear_out);
    usb_out_og_state.values = fd_indicator_animation_ease_linear_out;

    usb_run(0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f);
}

static
void usb_start_animation_powered_charging(void) {
    fd_indicator_animation_list_step_og_initialize(&usb_in_og_state);
    usb_in_og_state.size = sizeof(fd_indicator_animation_ease_linear_in);
    usb_in_og_state.values = fd_indicator_animation_ease_linear_in;

    fd_indicator_animation_list_step_og_initialize(&usb_cycle_og_state);
    usb_cycle_og_state.size = sizeof(fd_indicator_animation_ease_quad_pulse);
    usb_cycle_og_state.values = fd_indicator_animation_ease_quad_pulse;

    fd_indicator_animation_list_step_og_initialize(&usb_out_og_state);
    usb_out_og_state.size = sizeof(fd_indicator_animation_ease_linear_out);
    usb_out_og_state.values = fd_indicator_animation_ease_linear_out;

    usb_run(1.0f, 1.0f, 0.0f, 0.2f, 51.0f, 0.8f, 0.0f, 0.2f);
}

static
void usb_show(void) {
    switch (usb_condition_current) {
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
    usb_condition_showing = usb_condition_current;
}

void fd_indicator_set_usb_condition(fd_indicator_usb_condition_t condition) {
    if (usb_condition_current == condition) {
        return;
    }

    if (usb_condition_showing == condition) {
        fd_indicator_animation_run(&usb_animation);
        usb_condition_current = condition;
        return;
    }

    if (usb_animation.running) {
        usb_animation.cancelling = true;
        usb_condition_current = condition;
        return;
    }

    // start usb animation
    usb_condition_current = condition;

    usb_show();
}

void fd_indicator_step(void) {
    fd_indicator_animation_step(&usb_animation);
}

void fd_indicator_initialize(void) {
    usb_condition_showing = usb_condition_current = fd_indicator_usb_condition_unpowered;
    fd_indicator_animation_initialize(&usb_animation);

    fd_event_add_callback(FD_EVENT_RTC_TICK, fd_indicator_step);
}
