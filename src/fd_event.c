#include "fd_event.h"
#include "fd_hal_processor.h"
#include "fd_hal_reset.h"
#include "fd_log.h"

#include <string.h>

typedef struct {
    uint32_t events;
    fd_event_callback_t callback;
#ifdef FD_EVENT_TIMING
    fd_timing_t timing;
#endif
} fd_event_item_t;

#define ITEM_LIMIT 32

static
fd_event_item_t fd_event_items[ITEM_LIMIT];
static
uint32_t fd_event_item_count;

#define CHECK_LIMIT 4

static
fd_event_em2_check_t fd_event_em2_checks[CHECK_LIMIT];
static
uint32_t fd_event_em2_check_count;

volatile uint32_t fd_event_pending;

void fd_event_initialize(void) {
    fd_event_item_count = 0;
    fd_event_em2_check_count = 0;
    fd_event_pending = 0;
    memset(fd_event_items, 0, sizeof(fd_event_items));
}

void fd_event_add_em2_check(fd_event_em2_check_t em2_check) {
    if (fd_event_em2_check_count >= CHECK_LIMIT) {
        fd_log_assert_fail("");
        return;
    }

    fd_event_em2_checks[fd_event_em2_check_count++] = em2_check;
}

void fd_event_add_callback_with_identifier(uint32_t events, fd_event_callback_t callback, const char *identifier __attribute__((unused))) {
    if (fd_event_item_count >= ITEM_LIMIT) {
        fd_log_assert_fail("");
        return;
    }

    fd_event_item_t *item = &fd_event_items[fd_event_item_count++];
    item->events = events;
    item->callback = callback;
#ifdef FD_EVENT_TIMING
    fd_timing_initialize(&item->timing, identifier);
#endif
}

fd_timing_iterator_t fd_event_timing_iterator(void) {
#ifdef FD_EVENT_TIMING
    fd_timing_iterator_t iterator = fd_timing_iterator_array_of_objects(fd_event_item_t, timing, fd_event_items, fd_event_item_count);
#else
    fd_timing_iterator_t iterator = fd_timing_iterator_nil();
#endif
    return iterator;
}

void fd_event_set_exclusive(uint32_t events) {
    fd_hal_processor_interrupts_disable();
    fd_event_set(events);
    fd_hal_processor_interrupts_enable();
}

void fd_event_set(uint32_t events) {
    fd_event_pending |= events;
}

bool fd_event_process_pending(void) {
    fd_hal_processor_interrupts_disable();
    uint32_t pending = fd_event_pending;
    fd_event_pending = 0;
    fd_hal_processor_interrupts_enable();
    
    fd_hal_reset_feed_watchdog();
    
    if (pending) {
        bool is_timing = fd_hal_timing_get_enable();
        fd_event_item_t *item = fd_event_items;
        fd_event_item_t *end = &fd_event_items[fd_event_item_count];
        for (; item < end; ++item) {
            if (item->events & pending) {
                if (is_timing) {
                    fd_timing_start(&item->timing);
                }
                (*item->callback)();
                if (is_timing) {
                    fd_timing_end(&item->timing);
                }
            }
        }
    }
    return pending != 0;
}

void fd_event_process(void) {
    bool pending = fd_event_process_pending();
    if (!pending) {
        for (uint32_t i = 0; i < fd_event_em2_check_count; ++i) {
            fd_event_em2_check_t em2_check = fd_event_em2_checks[i];
            if (!em2_check()) {
                return;
            }
        }
        fd_hal_processor_wait();
    }
}

