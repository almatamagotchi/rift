/*
 * rift — FreeBSD platform backend
 *
 * Console input: keyboard via stdin (raw mode, tty ioctl)
 * Mouse: sysmouse(4) via /dev/sysmouse or console mouse ioctls
 */

#include "../src/common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/consio.h>
#include <sys/mouse.h>

static struct termios orig_termios;
static int term_cols, term_rows;
static int mouse_fd = -1;

int plat_init(void)
{
    struct termios raw;
    int cols, rows;

    /* get console size */
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &cols) == 0) {
        term_cols = cols;
        term_rows = rows;
    } else {
        /* fallback: try CONS_GETINFO */
        vid_info_t vi;
        if (ioctl(0, CONS_GETINFO, &vi) == 0) {
            term_cols = vi.mv_csz;
            term_rows = vi.mv_rsz;
        } else {
            term_cols = 80;
            term_rows = 24;
        }
    }

    /* save and set raw mode */
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    cfmakeraw(&raw);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* open sysmouse for mouse events */
    mouse_fd = open("/dev/sysmouse", O_RDONLY | O_NONBLOCK);
    if (mouse_fd < 0) {
        /* try console mouse fd */
        mouse_fd = open("/dev/consolectl", O_RDONLY | O_NONBLOCK);
    }

    /* enable mouse in console */
    int m = 1;
    ioctl(STDIN_FILENO, CONS_MOUSECTL, &m);

    /* hide cursor */
    write(STDOUT_FILENO, "\033[?25l", 6);

    return 0;
}

void plat_shutdown(void)
{
    int m = 0;
    ioctl(STDIN_FILENO, CONS_MOUSECTL, &m);

    if (mouse_fd >= 0) close(mouse_fd);

    write(STDOUT_FILENO, "\033[?25h", 6);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);
}

int plat_get_term_size(int *cols, int *rows)
{
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_cols = ws.ws_col;
        term_rows = ws.ws_row;
    }
    *cols = term_cols;
    *rows = term_rows;
    return 0;
}

int plat_read_input(char *buf, size_t max)
{
    ssize_t n = read(STDIN_FILENO, buf, max);
    return n > 0 ? (int)n : 0;
}

void plat_render_begin(void)  { }
void plat_render_end(void)    { fflush(stdout); }

void plat_render_flush(void)
{
    fflush(stdout);
}

void plat_cursor_to(int col, int row)
{
    printf("\033[%d;%dH", row + 1, col + 1);
}

void plat_putch(int ch)
{
    putchar(ch);
}

void plat_puts(const char *s)
{
    fputs(s, stdout);
}

void plat_attr(int fg, int bg, int flags)
{
    printf("\033[0");
    if (flags & 1)  printf(";1");
    if (flags & 2)  printf(";2");
    if (flags & 4)  printf(";3");
    if (flags & 8)  printf(";4");
    if (flags & 16) printf(";7");
    if (fg >= 0)    printf(";38;5;%d", fg);
    if (bg >= 0)    printf(";48;5;%d", bg);
    putchar('m');
}

void plat_clear(void)
{
    write(STDOUT_FILENO, "\033[2J", 4);
}
