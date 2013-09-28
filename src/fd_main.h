#ifndef FD_MAIN_H
#define FD_MAIN_H

typedef enum {fd_main_mode_run, fd_main_mode_storage} fd_main_mode_t;

fd_main_mode_t fd_main_get_mode(void);
void fd_main_set_mode(fd_main_mode_t mode);

#endif