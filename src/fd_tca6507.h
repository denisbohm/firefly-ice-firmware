#ifndef FD_TCA6507_H
#define FD_TCA6507_H

void fd_tca6507_initialize(void);

void fd_tca6507_enable(void);
void fd_tca6507_disable(void);

void fd_tca6507_test(void);

void fd_tca6507_set_color(bool r, bool g, bool b);

#endif