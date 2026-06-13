/*
 * render.c — screen rendering
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>

static void win_draw_content(const Window *win)
{
    if (!win || !win->term) return;

    int start_y = win->inner.y;
    int end_y   = start_y + win->inner.h;

    for (int row = 0; row < win->inner.h && start_y + row < end_y; row++) {
        plat_cursor_to(win->inner.x, start_y + row);
        for (int col = 0; col < win->inner.w; col++) {
            const Cell *cell = term_cell((Term *)win->term, col, row);
            if (cell && cell->ch) {
                char mb[8] = {0};
                if (cell->ch < 0x80) {
                    mb[0] = (char)cell->ch;
                } else if (cell->ch < 0x800) {
                    mb[0] = 0xC0 | (char)(cell->ch >> 6);
                    mb[1] = 0x80 | (char)(cell->ch & 0x3F);
                } else {
                    mb[0] = 0xE0 | (char)(cell->ch >> 12);
                    mb[1] = 0x80 | (char)((cell->ch >> 6) & 0x3F);
                    mb[2] = 0x80 | (char)(cell->ch & 0x3F);
                }
                plat_puts(mb);
            } else {
                plat_putch(' ');
            }
        }
    }
}

void wm_render(void)
{
    plat_render_begin();
    plat_clear();

    /* render all visible windows sorted by z (bottom-to-top) */
    int count = win_count();
    Window **sorted = calloc(count, sizeof(Window *));
    if (!sorted) return;

    int n = 0;
    for (Window *w = win_first(); w; w = w->next) {
        if (n < count) sorted[n++] = w;
    }

    /* bubble sort by z (small n) */
    for (int a = 0; a < n; a++) {
        for (int b = a + 1; b < n; b++) {
            if (sorted[a]->z > sorted[b]->z) {
                Window *tmp = sorted[a];
                sorted[a] = sorted[b];
                sorted[b] = tmp;
            }
        }
    }

    for (int i = 0; i < n; i++) {
        Window *w = sorted[i];
        if (!w->visible) continue;
        win_draw_content(w);
        win_draw_frame(w);
    }

    free(sorted);
    plat_render_end();
}
