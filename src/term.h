/*
 * term.h — terminal emulator buffer with scrollback
 */
#ifndef RIFT_TERM_H
#define RIFT_TERM_H

#include "common.h"
#include <stddef.h>

#define TERM_TAB_STOP 8
#define TERM_SCROLLBACK 2000   /* lines of scrollback */

typedef struct {
    wchar_t  ch;       /* character (0 = empty) */
    int      fg;        /* -1 = default */
    int      bg;        /* -1 = default */
    int      flags;     /* ATTR_BOLD, etc. */
} Cell;

typedef struct Term {
    Cell    *cells;     /* ring buffer: (scrollback + rows) × cols */
    int      cap;       /* total capacity in rows (scrollback + rows) */
    int      offset;    /* scroll position (0 = bottom) */
    int      cols;
    int      rows;      /* visible rows (screen height) */
    int      line_count;/* total lines stored (≤ cap) */
    int      cur_x, cur_y;  /* cursor position within visible area */
    bool     dirty;
} Term;

Term *term_create(int cols, int rows);
void  term_destroy(Term *t);
void  term_resize(Term *t, int cols, int rows);
void  term_write(Term *t, const char *data, int len);
void  term_scroll_up(Term *t, int n);
void  term_scroll_down(Term *t, int n);
Cell *term_cell(Term *t, int col, int row);  /* row 0 = top of visible */

#endif
