/*
 * bar.c — taskbar with launcher and clock
 */
#include "common.h"
#include <stdio.h>
#include <string.h>
#include <time.h>

void bar_init(Bar *bar)
{
    memset(bar, 0, sizeof(*bar));
    bar->visible = true;
    bar->height  = 1;
}

void bar_render(const Bar *bar, int cols)
{
    /* TODO: draw bar at bottom row with:
     *   [Launcher input | -- task buttons -- | clock]
     */
    (void)bar;
    (void)cols;
}

int bar_handle_click(Bar *bar, int x, int y, int btn)
{
    /* TODO: handle clicks on task buttons, launcher */
    (void)bar;
    (void)x;
    (void)y;
    (void)btn;
    return 0;
}

void bar_update_clock(Bar *bar)
{
    time_t now = time(NULL);
    if (now == bar->last_clock) return;
    bar->last_clock = now;
    struct tm *tm = localtime(&now);
    strftime(bar->clock_str, sizeof(bar->clock_str), "%H:%M", tm);
}
