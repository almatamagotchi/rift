/*
 * rift — TUI window manager: main entry point
 */
#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>

static int g_cols, g_rows;
static int running = 1;
static Window *g_shell = NULL;  /* primary shell window */

/* input parser (from input.c) */
int input_parse(const char *buf, int len, Event *out, int max_events);

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
}

static void pty_poll(Window *win)
{
    if (!win || !win->pty || !win->term) return;
    char buf[4096];
    int n = pty_read(win->pty, buf, sizeof(buf));
    while (n > 0) {
        term_write(win->term, buf, n);
        n = pty_read(win->pty, buf, sizeof(buf));
    }
}

int main(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    signal(SIGINT,  sig_handler);
    signal(SIGTERM, sig_handler);
    signal(SIGHUP,  sig_handler);
    signal(SIGQUIT, sig_handler);

    if (plat_init() != 0) {
        fprintf(stderr, "rift: platform init failed\n");
        return 1;
    }

    plat_get_term_size(&g_cols, &g_rows);

    if (wm_init(g_cols, g_rows) != 0) {
        fprintf(stderr, "rift: wm init failed\n");
        plat_shutdown();
        return 1;
    }

    char inbuf[256];

    while (running) {
        /* poll PTY output from all windows */
        for (Window *w = win_first(); w; w = w->next) {
            pty_poll(w);
        }

        int n = plat_read_input(inbuf, sizeof(inbuf));
        if (n > 0) {
            /* parse input and dispatch to focused window */
            #define MAX_EVENTS 16
            Event events[MAX_EVENTS];
            int nev = input_parse(inbuf, n, events, MAX_EVENTS);
            for (int i = 0; i < nev; i++) {
                wm_handle_event(&events[i]);
            }
        }

        wm_render();
        plat_render_flush();

        struct timespec ts = { .tv_sec = 0, .tv_nsec = 16666667 };
        nanosleep(&ts, NULL);
    }

    wm_shutdown();
    plat_shutdown();

    return 0;
}

int wm_init(int cols, int rows)
{
    /* create initial shell window filling the screen */
    g_shell = win_create("shell", 0, 0, cols, rows, "/bin/sh");
    if (!g_shell) return -1;
    win_focus(g_shell);
    return 0;
}

void wm_shutdown(void)
{
    while (win_first()) {
        win_destroy(win_first());
    }
}

/* write a string to a PTY */
static void pty_write_str(Pty *pty, const char *s)
{
    pty_write(pty, s, strlen(s));
}

/* send ANSI CSI sequence: ESC [ <optional param> <letter>
 * format with modifiers: ESC [ 1 ; mod <letter>
 * mod encoding (xterm style): shift=2, alt=3, ctrl=5
 * combos: shift+alt=4, ctrl+shift=6, ctrl+alt=7, ctrl+shift+alt=8
 */
static void write_ansi_csi(Window *win, const char *letter, int mod)
{
    if (mod == 0) {
        char buf[8];
        snprintf(buf, sizeof(buf), "\x1b[%s", letter);
        pty_write_str(win->pty, buf);
    } else {
        int m = 1;
        if (mod & MOD_SHIFT) m += 1;
        if (mod & MOD_ALT)   m += 2;
        if (mod & MOD_CTRL)  m += 4;
        char buf[16];
        snprintf(buf, sizeof(buf), "\x1b[1;%d%s", m, letter);
        pty_write_str(win->pty, buf);
    }
}

void wm_handle_event(const Event *ev)
{
    Window *f = wm_focused();
    if (!f || !f->pty) return;

    if (ev->type == EV_KEY) {
        /* forward printable ASCII and common controls to PTY */
        int key = ev->key.key;
        char c;

        if (key >= 0x20 && key < 0x7f) {
            c = (char)key;
            pty_write(f->pty, &c, 1);
        } else if (key == K_ENTER) {
            c = '\r';
            pty_write(f->pty, &c, 1);
        } else if (key == K_TAB) {
            c = '\t';
            pty_write(f->pty, &c, 1);
        } else if (key == K_BSP) {
            c = '\x7f';
            pty_write(f->pty, &c, 1);
        } else if (key == K_ESC) {
            c = '\x1b';
            pty_write(f->pty, &c, 1);
        }

        /* arrow keys — send ANSI escape sequences */
        else if (key == K_UP)    write_ansi_csi(f, "A", ev->key.mod);
        else if (key == K_DOWN)  write_ansi_csi(f, "B", ev->key.mod);
        else if (key == K_RIGHT) write_ansi_csi(f, "C", ev->key.mod);
        else if (key == K_LEFT)  write_ansi_csi(f, "D", ev->key.mod);

        /* navigation keys */
        else if (key == K_HOME)  write_ansi_csi(f, "H", ev->key.mod);
        else if (key == K_END)   write_ansi_csi(f, "F", ev->key.mod);
        else if (key == K_PGUP)  pty_write_str(f->pty, "\x1b[5~");
        else if (key == K_PGDN)  pty_write_str(f->pty, "\x1b[6~");

        /* ctrl + printable: map to control characters */
        else if ((ev->key.mod & MOD_CTRL) && key >= 'a' && key <= 'z') {
            c = (char)(key - 'a' + 1);
            pty_write(f->pty, &c, 1);
        }
        else if ((ev->key.mod & MOD_CTRL) && key >= 'A' && key <= 'Z') {
            c = (char)(key - 'A' + 1);
            pty_write(f->pty, &c, 1);
        }
        /* ctrl + other: common mappings */
        else if ((ev->key.mod & MOD_CTRL) && key == ' ') {
            c = '\0';  /* ctrl-space -> NUL */
            pty_write(f->pty, &c, 1);
        }
        else if ((ev->key.mod & MOD_CTRL) && key == '/') {
            c = '\x1f';  /* ctrl-/ -> US */
            pty_write(f->pty, &c, 1);
        }
        else if ((ev->key.mod & MOD_CTRL) && key == '[') {
            c = '\x1b';  /* ctrl-[ -> ESC */
            pty_write(f->pty, &c, 1);
        }
        else if ((ev->key.mod & MOD_CTRL) && key == '\\') {
            c = '\x1c';  /* ctrl-\ -> FS */
            pty_write(f->pty, &c, 1);
        }

        /* function keys F1-F12 */
        else if (key >= K_F1 && key <= K_F12) {
            char buf[8];
            snprintf(buf, sizeof(buf), "\x1b[%d~", key - K_F1 + 11);
            pty_write_str(f->pty, buf);
        }
        /* TODO: handle arrow keys, ctrl combos, etc. */
    }
}
