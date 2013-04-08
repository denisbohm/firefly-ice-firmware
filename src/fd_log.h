#ifndef FD_LOG_H
#define FD_LOG_H

#include <stdbool.h>

extern bool fd_log_did_log;

void fd_log_initialize(void);

void fd_log(char *message);

void fd_log_ram(char *message);

#define fd_log_string_helper(a) #a
#define fd_log_string(a) fd_log_string_helper(a)

#define fd_log_assert_fail(message) fd_log(__FILE__ ":" fd_log_string(__LINE__) ":" message)
#define fd_log_assert(condition) if (!(condition)) fd_log_assert_fail(#condition)

#define fd_log_ram_assert_fail(message) fd_log_ram(__FILE__ ":" fd_log_string(__LINE__) ":" message)
#define fd_log_ram_assert(condition) if (!(condition)) fd_log_ram_assert_fail(#condition)

#endif