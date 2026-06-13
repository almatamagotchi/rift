#ifndef RIFT_WINDOW_H
#define RIFT_WINDOW_H

#include "common.h"
#include <stddef.h>

struct Window {
    char   *title;
    Rect    rect;          /* outer rect including decorations */
    Rect    inner;         /* client area */
    WinState state;
    Pty    *pty;           /* null for non-terminal windows */
    Term   *term;          /* terminal buffer (null if no pty) */
    bool    visible;
    bool    focused;
    int     z;             /* stacking order */

    struct Window *next;
    struct Window *prev;
};

/* window list */
Window *win_create(const char *title, int x, int y, int w, int h, const char *cmd);
void    win_destroy(Window *win);
Window *win_at(Point p);
Window *win_by_index(int idx);
int     win_count(void);
Window *win_first(void);
Window *win_last(void);

void    win_move(Window *win, int dx, int dy);
void    win_resize(Window *win, int dw, int dh, bool from_corner);
void    win_raise(Window *win);
void    win_focus(Window *win);
void    win_maximize(Window *win);
void    win_restore(Window *win);
void    win_minimize(Window *win);
void    win_unminimize(Window *win);

/* rendering helpers */
void    win_draw_frame(const Window *win);
void    win_draw_title(const Window *win);

#endif /* RIFT_WINDOW_H */
