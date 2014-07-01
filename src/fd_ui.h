#ifndef FD_UI_H
#define FD_UI_H

#include "fd_lock.h"
#include <stdbool.h>

void fd_ui_initialize(void);

void fd_ui_update(void);

bool fd_ui_get_indicate(fd_lock_owner_t owner);
void fd_ui_set_indicate(fd_lock_owner_t owner, bool indicate);

#endif