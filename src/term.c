/*
 * term.c — terminal emulator buffer with scrollback
 */
#include "common.h"
#include "term.h"
#include <stdlib.h>
#include <string.h>
#include <wchar.h>

static inline int row_to_idx(const Term *t, int row)
{
    /* row 0 = bottom of ring (scrollback area), row (line_count-1) = newest */
    return row % t->cap;
}

static void ensure_capacity(Term *t, int needed)
{
    if (needed <= t->cap) return;
    int new_cap = needed * 2;
    Cell *new_cells = calloc((size_t)new_cap * t->cols, sizeof(Cell));
    if (!new_cells) return;

    /* copy old cells: old ring -> new ring at same positions */
    int start = t->line_count - t->offset >= t->cap ? 0
                      : t->line_count - t->cap;
    if (start < 0) start = 0;
    int actual = t->line_count - start;
    if (actual > t->cap) actual = t->cap;
    if (actual < 0) actual = 0;

    for (int i = 0; i < actual; i++) {
        int old_row = start + i;
        int old_idx = row_to_idx(t, old_row);
        int new_idx = old_row % new_cap;
        memcpy(&new_cells[new_idx * t->cols],
               &t->cells[old_idx * t->cols],
               t->cols * sizeof(Cell));
    }

    free(t->cells);
    t->cells = new_cells;
    t->cap = new_cap;
}

Term *term_create(int cols, int rows)
{
    Term *t = calloc(1, sizeof(Term));
    if (!t) return NULL;
    t->cols  = cols > 0 ? cols : 80;
    t->rows  = rows > 0 ? rows : 24;
    t->cap   = TERM_SCROLLBACK + t->rows;
    t->cells = calloc((size_t)t->cap * t->cols, sizeof(Cell));
    if (!t->cells) { free(t); return NULL; }
    t->cur_x = 0;
    t->cur_y = 0;
    t->dirty = true;
    return t;
}

void term_destroy(Term *t)
{
    if (!t) return;
    free(t->cells);
    free(t);
}

void term_resize(Term *t, int cols, int rows)
{
    if (!t || (cols == t->cols && rows == t->rows)) return;

    int new_cap = TERM_SCROLLBACK + rows;
    Cell *new_cells = calloc((size_t)new_cap * cols, sizeof(Cell));
    if (!new_cells) return;

    /* best-effort copy: keep last rows lines, clipping columns */
    int copy_rows = t->line_count < rows ? t->line_count : rows;
    for (int r = 0; r < copy_rows; r++) {
        int src_row = t->line_count - copy_rows + r;
        int src_idx = row_to_idx(t, src_row);
        int dst_idx = r % new_cap;
        int copy_cols = t->cols < cols ? t->cols : cols;
        memcpy(&new_cells[dst_idx * cols],
               &t->cells[src_idx * t->cols],
               copy_cols * sizeof(Cell));
    }

    free(t->cells);
    t->cells = new_cells;
    t->cap   = new_cap;
    t->cols  = cols;
    t->rows  = rows;
    t->line_count = copy_rows;
    t->offset = 0;
    if (t->cur_x >= cols)  t->cur_x = cols - 1;
    if (t->cur_y >= rows)  t->cur_y = rows - 1;
    t->dirty = true;
}

static void term_putc(Term *t, wchar_t ch)
{
    /* ensure row exists in ring */
    while (t->cur_y >= t->line_count) {
        int idx = row_to_idx(t, t->line_count);
        memset(&t->cells[idx * t->cols], 0, t->cols * sizeof(Cell));
        t->line_count++;
        if (t->line_count > t->cap) {
            /* ring wrapped — drop oldest */
            int drop_idx = row_to_idx(t, t->line_count - t->cap - 1);
            memset(&t->cells[drop_idx * t->cols], 0, t->cols * sizeof(Cell));
        }
    }

    int idx = row_to_idx(t, t->cur_y);
    Cell *cell = &t->cells[idx * t->cols + t->cur_x];
    cell->ch = ch;
    cell->fg = -1;
    cell->bg = -1;
    cell->flags = 0;

    t->dirty = true;
}

void term_write(Term *t, const char *data, int len)
{
    if (!t || len <= 0) return;

    for (int i = 0; i < len; i++) {
        char c = data[i];

        switch (c) {
        case '\r':
            t->cur_x = 0;
            break;
        case '\n':
            t->cur_x = 0;
            t->cur_y++;
            /* scroll up if past bottom */
            if (t->cur_y >= t->rows) {
                t->cur_y = t->rows - 1;
                term_scroll_up(t, 1);
            }
            break;
        case '\t': {
            int ts = TERM_TAB_STOP - (t->cur_x % TERM_TAB_STOP);
            for (int j = 0; j < ts && t->cur_x < t->cols; j++)
                term_putc(t, L' ');
            break;
        }
        case '\b':
            if (t->cur_x > 0) t->cur_x--;
            break;
        case '\x1b':
            /* skip ESC sequences (naive: consume until letter) */
            i++; /* skip '[' or other */
            while (i < len && data[i] >= 0x20 && data[i] <= 0x3f) i++;
            while (i < len && data[i] >= 0x40 && data[i] <= 0x7e) i++;
            i--; /* loop will increment */
            break;
        default:
            if ((unsigned char)c >= 0x20) {
                if (t->cur_x >= t->cols) {
                    t->cur_x = 0;
                    t->cur_y++;
                    if (t->cur_y >= t->rows) {
                        t->cur_y = t->rows - 1;
                        term_scroll_up(t, 1);
                    }
                }
                term_putc(t, (wchar_t)(unsigned char)c);
                t->cur_x++;
            }
            break;
        }
    }
}

void term_scroll_up(Term *t, int n)
{
    if (!t) return;
    /* add n blank lines at bottom */
    for (int i = 0; i < n; i++) {
        if (t->line_count >= t->cap) {
            /* ring full — oldest gets overwritten */
            int old_idx = row_to_idx(t, t->line_count - t->cap);
            memset(&t->cells[old_idx * t->cols], 0, t->cols * sizeof(Cell));
        }
        t->line_count++;
    }
    t->dirty = true;
}

void term_scroll_down(Term *t, int n)
{
    (void)t; (void)n;
    /* TODO: scrollback navigation */
}

Cell *term_cell(Term *t, int col, int row)
{
    if (!t || col < 0 || col >= t->cols || row < 0 || row >= t->rows)
        return NULL;

    int scroll_off = t->offset;
    int line_idx = t->line_count - t->rows + row - scroll_off;
    if (line_idx < 0 || line_idx >= t->line_count) return NULL;

    int idx = row_to_idx(t, line_idx);
    return &t->cells[idx * t->cols + col];
}
