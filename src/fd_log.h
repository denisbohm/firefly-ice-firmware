#ifndef FD_LOG_H
#define FD_LOG_H

#include <stdbool.h>
#include <stdint.h>

extern bool fd_log_did_log;

void fd_log_initialize(void);

void fd_log_set_count(uint32_t count);
uint32_t fd_log_get_count(void);

bool fd_log_get_storage(void);
void fd_log_set_storage(bool enable);

void fd_log(char *message);
void fd_log_at(char *file, int line, char *message);

#define fd_log_string_helper(a) #a
#define fd_log_string(a) fd_log_string_helper(a)

#if defined(FD_ASSERT_NONE)
#define fd_log_assert_fail(message)
#define fd_log_assert(condition)
#elif defined(FD_ASSERT_TERSE)
#define fd_log_assert_fail(message) fd_log_at(__FILE__, __LINE__, "")
#define fd_log_assert(condition) if (!(condition)) fd_log_assert_fail(#condition)
#else
#define fd_log_assert_fail(message) fd_log_at(__FILE__, __LINE__, message)
#define fd_log_assert(condition) if (!(condition)) fd_log_assert_fail(#condition)
#endif

#endif