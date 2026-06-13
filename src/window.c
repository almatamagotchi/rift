/*
 * window.c — window management
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>

static Window *win_list      = NULL;
static Window *focused_win   = NULL;
static int     win_z_top     = 0;

Window *win_create(const char *title, int x, int y, int w, int h, const char *cmd)
{
    Window *win = calloc(1, sizeof(Window));
    if (!win) return NULL;

    win->title  = strdup(title ? title : "rift");
    win->rect.x = x;
    win->rect.y = y;
    win->rect.w = w;
    win->rect.h = h;
    win->inner  = win->rect;
    win->state  = WIN_NORMAL;
    win->visible = true;
    win->z       = ++win_z_top;

    /* allocate term buffer and spawn pty if a command is given */
    if (cmd && *cmd) {
        win->pty = pty_spawn(cmd, w, h);
        win->term = term_create(w, h);
    }

    /* insert at head of list */
    win->next = win_list;
    win->prev = NULL;
    if (win_list) win_list->prev = win;
    win_list = win;

    return win;
}

void win_destroy(Window *win)
{
    if (!win) return;
    if (win == focused_win) focused_win = NULL;

    if (win->pty) pty_destroy(win->pty);
    if (win->term) term_destroy(win->term);

    if (win->prev) win->prev->next = win->next;
    if (win->next) win->next->prev = win->prev;
    if (win_list == win) win_list = win->next;

    free(win->title);
    free(win);
}

Window *win_at(Point p)
{
    /* return topmost window at point (highest z) */
    Window *best = NULL;
    int best_z = -1;
    for (Window *w = win_list; w; w = w->next) {
        if (!w->visible) continue;
        if (p.x >= w->rect.x && p.x < w->rect.x + (int)w->rect.w &&
            p.y >= w->rect.y && p.y < w->rect.y + (int)w->rect.h) {
            if (w->z > best_z) { best = w; best_z = w->z; }
        }
    }
    return best;
}

Window *win_by_index(int idx)
{
    for (Window *w = win_list; w; w = w->next) {
        if (idx-- == 0) return w;
    }
    return NULL;
}

int win_count(void)
{
    int n = 0;
    for (Window *w = win_list; w; w = w->next) n++;
    return n;
}

Window *win_first(void) { return win_list; }
Window *win_last(void)
{
    Window *w = win_list;
    while (w && w->next) w = w->next;
    return w;
}

void win_move(Window *win, int dx, int dy)
{
    win->rect.x  += dx;
    win->rect.y  += dy;
    win->inner.x += dx;
    win->inner.y += dy;
}

void win_resize(Window *win, int dw, int dh, bool from_corner)
{
    (void)from_corner;
    win->rect.w += dw;
    win->rect.h += dh;
    if (win->rect.w < MIN_WIN_W) win->rect.w = MIN_WIN_W;
    if (win->rect.h < MIN_WIN_H) win->rect.h = MIN_WIN_H;
    win->inner.w = win->rect.w;
    win->inner.h = win->rect.h;
    if (win->pty) pty_resize(win->pty, win->inner.w, win->inner.h);
    if (win->term) term_resize(win->term, win->inner.w, win->inner.h);
}

void win_raise(Window *win)
{
    if (!win) return;
    win->z = ++win_z_top;
}

void win_focus(Window *win)
{
    if (focused_win) focused_win->focused = false;
    focused_win = win;
    if (focused_win) {
        focused_win->focused = true;
        win_raise(focused_win);
    }
}

Window *wm_focused(void)
{
    return focused_win;
}

void win_maximize(Window *win)  { (void)win; /* TODO */ }
void win_restore(Window *win)   { (void)win; /* TODO */ }
void win_minimize(Window *win)  { (void)win; /* TODO */ }
void win_unminimize(Window *win){ (void)win; /* TODO */ }

void win_draw_frame(const Window *win)
{
    /* TODO: draw box-drawing border + title bar */
    (void)win;
}

void win_draw_title(const Window *win)
{
    /* TODO: draw centered/left-aligned title */
    (void)win;
}
