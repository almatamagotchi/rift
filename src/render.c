/*
 * render.c — screen rendering
 */
#include "common.h"
#include <string.h>

void wm_render(void)
{
    Window *w;

    plat_clear();

    /* walk bottom-to-top to draw stacked windows */
    for (w = win_first(); w; w = w->next) {
        if (!w->visible) continue;
        /* TODO: draw window decorations and content */
    }

    /* TODO: draw bar at bottom */
}
