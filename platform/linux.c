/*
 * rift — Linux platform backend
 *
 * Console input: keyboard via stdin (raw mode, tty ioctl)
 * Mouse: GPM (General Purpose Mouse) library
 *
 * Build: needs libgpm-dev on Debian/Ubuntu
 */

#define _GNU_SOURCE
#include "../src/common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <signal.h>

#ifdef RIFT_GPM
#include <gpm.h>
#endif

static struct termios orig_termios;
static int term_cols, term_rows;

int plat_init(void)
{
    struct termios raw;

    /* get initial size */
    struct winsize ws;
    if (ioctl(STDIN_FILENO, TIOCGWINSZ, &ws) == 0) {
        term_cols = ws.ws_col;
        term_rows = ws.ws_row;
    } else {
        term_cols = 80;
        term_rows = 24;
    }

    /* save and set raw mode */
    tcgetattr(STDIN_FILENO, &orig_termios);
    raw = orig_termios;
    cfmakeraw(&raw);
    raw.c_cc[VMIN]  = 0;
    raw.c_cc[VTIME] = 1;
    tcsetattr(STDIN_FILENO, TCSANOW, &raw);

    /* enable mouse tracking via xterm escape sequences */
    write(STDOUT_FILENO, "\033[?1000h\033[?1002h\033[?1003h", 21);

    /* hide cursor, use alternate screen */
    write(STDOUT_FILENO, "\033[?25l\033[?1049h", 14);

#ifdef RIFT_GPM
    /* init GPM for console mouse */
    Gpm_Connect conn;
    conn.eventMask   = GPM_DOWN | GPM_UP | GPM_DRAG | GPM_MOVE;
    conn.defaultMask = 0;
    conn.minMod      = 0;
    conn.maxMod      = ~0;
    Gpm_Open(&conn, 0);
#endif

    return 0;
}

void plat_shutdown(void)
{
    write(STDOUT_FILENO, "\033[?25h\033[?1049l\033[?1003l\033[?1002l\033[?1000l", 33);
    tcsetattr(STDIN_FILENO, TCSANOW, &orig_termios);

#ifdef RIFT_GPM
    Gpm_Close();
#endif
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

void plat_render_begin(void)  { /* alternate screen already active */ }
void plat_render_end(void)    { fflush(stdout); }

void plat_render_flush(void)
{
    /* write accumulated buffer to stdout */
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
    /* ANSI 256-color + attributes
     * flags: bit 0 = bold, bit 1 = dim, bit 2 = italic,
     *        bit 3 = underline, bit 4 = reverse
     */
    printf("\033[0");  /* reset */
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
