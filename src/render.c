/*
 * render.c — screen rendering with dirty-cell tracking
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>

/* back-buffer: what we last painted to the physical screen */
typedef struct {
    wchar_t ch;
    int fg, bg;
    uint8_t flags;
    bool valid;
} ScreenCell;

static ScreenCell *screen_buf = NULL;
static int screen_cols = 0;
static int screen_rows = 0;

static void screen_ensure(int cols, int rows)
{
    if (screen_buf && screen_cols == cols && screen_rows == rows) return;
    free(screen_buf);
    screen_buf = calloc((size_t)cols * rows, sizeof(ScreenCell));
    screen_cols = cols;
    screen_rows = rows;
}

/* emit one UTF-8 char to stdout without cursor movement (used for dirty-diff writes) */
static void emit_cell(wchar_t ch)
{
    if (ch < 0x80) {
        plat_putch((char)ch);
    } else if (ch < 0x800) {
        char mb[3];
        mb[0] = 0xC0 | (char)(ch >> 6);
        mb[1] = 0x80 | (char)(ch & 0x3F);
        mb[2] = '\0';
        plat_puts(mb);
    } else if (ch < 0x10000) {
        char mb[4];
        mb[0] = 0xE0 | (char)(ch >> 12);
        mb[1] = 0x80 | (char)((ch >> 6) & 0x3F);
        mb[2] = 0x80 | (char)(ch & 0x3F);
        mb[3] = '\0';
        plat_puts(mb);
    } else {
        char mb[5];
        mb[0] = 0xF0 | (char)(ch >> 18);
        mb[1] = 0x80 | (char)((ch >> 12) & 0x3F);
        mb[2] = 0x80 | (char)((ch >> 6) & 0x3F);
        mb[3] = 0x80 | (char)(ch & 0x3F);
        mb[4] = '\0';
        plat_puts(mb);
    }
}

/* write a cell only if it differs from the back-buffer, then update back-buffer */
static void dirty_put(int sx, int sy, wchar_t ch, int fg, int bg, uint8_t flags)
{
    if (sx < 0 || sx >= screen_cols || sy < 0 || sy >= screen_rows) return;
    ScreenCell *sc = &screen_buf[sy * screen_cols + sx];
    if (sc->valid && sc->ch == ch && sc->fg == fg && sc->bg == bg && sc->flags == flags)
        return;
    plat_cursor_to(sx, sy);
    if (ch) {
        emit_cell(ch);
    } else {
        plat_putch(' ');
    }
    sc->ch = ch;
    sc->fg = fg;
    sc->bg = bg;
    sc->flags = flags;
    sc->valid = true;
}

static void win_draw_content_diff(Window *win)
{
    if (!win || !win->term) return;

    for (int row = 0; row < win->inner.h; row++) {
        for (int col = 0; col < win->inner.w; col++) {
            const Cell *cell = term_cell((Term *)win->term, col, row);
            int sx = win->inner.x + col;
            int sy = win->inner.y + row;
            if (cell && cell->ch) {
                dirty_put(sx, sy, cell->ch, cell->fg, cell->bg, cell->flags);
            } else {
                dirty_put(sx, sy, 0, -1, -1, 0);
            }
        }
    }
}

void wm_render(void)
{
    plat_render_begin();
    int cols, rows;
    plat_get_term_size(&cols, &rows);
    screen_ensure(cols, rows);

    /* render all visible windows sorted by z (bottom-to-top) */
    int count = win_count();
    Window **sorted = calloc(count, sizeof(Window *));
    if (!sorted) { plat_render_end(); return; }

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

    /* first pass: draw content for visible windows */
    for (int i = 0; i < n; i++) {
        Window *w = sorted[i];
        if (!w->visible) continue;
        win_draw_content_diff(w);
    }

    /* second pass: draw frames (always redraw — frames are cheap and change often) */
    for (int i = 0; i < n; i++) {
        Window *w = sorted[i];
        if (!w->visible) continue;
        win_draw_frame(w);
    }

    /* clear any screen cells no longer covered by any window */
    for (int row = 0; row < screen_rows; row++) {
        for (int col = 0; col < screen_cols; col++) {
            bool covered = false;
            for (int i = 0; i < n && !covered; i++) {
                Window *w = sorted[i];
                if (!w->visible) continue;
                if (col >= w->rect.x && col < w->rect.x + w->rect.w &&
                    row >= w->rect.y && row < w->rect.y + w->rect.h) {
                    covered = true;
                }
            }
            if (!covered) {
                dirty_put(col, row, 0, -1, -1, 0);
            }
        }
    }

    free(sorted);
    plat_render_end();
}
