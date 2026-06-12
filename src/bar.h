#ifndef RIFT_BAR_H
#define RIFT_BAR_H

#include "common.h"
#include <time.h>

struct Bar {
    bool    visible;
    int     height;       /* 1 row */
    char    launcher_buf[256];
    int     launcher_len;
    bool    launcher_active;
    time_t  last_clock;
    char    clock_str[32];
};

void bar_init(Bar *bar);
void bar_render(const Bar *bar, int cols);
int  bar_handle_click(Bar *bar, int x, int y, int btn);
void bar_update_clock(Bar *bar);

#endif /* RIFT_BAR_H */
