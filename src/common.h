#ifndef RIFT_COMMON_H
#define RIFT_COMMON_H

#define _POSIX_C_SOURCE 200809L
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>

#define RIFT_VERSION "0.0.1"

/* window constraints */
#define MIN_WIN_W 8
#define MIN_WIN_H 3

/* box-drawing characters */
#define BOX_H   0x2500  /* ─ */
#define BOX_V   0x2502  /* │ */
#define BOX_TL  0x250c  /* ┌ */
#define BOX_TR  0x2510  /* ┐ */
#define BOX_BL  0x2514  /* └ */
#define BOX_BR  0x2518  /* ┘ */
/* double-line variants */
#define BOX_HD  0x2550  /* ═ */
#define BOX_VD  0x2551  /* ║ */
#define BOX_TLD 0x2554  /* ╔ */
#define BOX_TRD 0x2557  /* ╗ */
#define BOX_BLD 0x255a  /* ╚ */
#define BOX_BRD 0x255d  /* ╝ */

/*
 * rect: top-left corner + width + height
 * coordinates are character cells (cols, rows)
 */
typedef struct {
    int16_t x;
    int16_t y;
    uint16_t w;
    uint16_t h;
} Rect;

/*
 * point:
 */
typedef struct {
    int16_t x;
    int16_t y;
} Point;

/*
 * window state
 */
typedef enum {
    WIN_NORMAL,
    WIN_MAXIMIZED,
    WIN_SNAPPED_LEFT,
    WIN_SNAPPED_RIGHT,
    WIN_SNAPPED_TOP,
    WIN_SNAPPED_BOTTOM,
    WIN_MINIMIZED
} WinState;

typedef struct Window Window;
typedef struct Pty Pty;
typedef struct Bar Bar;

#include "window.h"
#include "pty.h"
#include "bar.h"
#include "snap.h"

/* platform abstraction */
int  plat_init(void);
void plat_shutdown(void);
int  plat_get_term_size(int *cols, int *rows);
int  plat_read_input(char *buf, size_t max);
void plat_render_begin(void);
void plat_render_end(void);
void plat_render_flush(void);
void plat_cursor_to(int col, int row);
void plat_putch(int ch);
void plat_puts(const char *s);
void plat_attr(int fg, int bg, int flags);
void plat_clear(void);

/* input event types */
typedef enum {
    EV_NONE,
    EV_KEY,
    EV_MOUSE_MOVE,
    EV_MOUSE_DOWN,
    EV_MOUSE_UP,
    EV_MOUSE_WHEEL_UP,
    EV_MOUSE_WHEEL_DOWN,
    EV_RESIZE
} EvType;

typedef struct {
    EvType type;
    union {
        struct { int key; int mod; } key;
        struct { int x, y; int btn; } mouse;
        struct { int cols, rows; } resize;
    };
} Event;

/* main event loop */
int  wm_init(int cols, int rows);
void wm_shutdown(void);
void wm_handle_event(const Event *ev);
void wm_render(void);
Window *wm_focused(void);

#endif /* RIFT_COMMON_H */
