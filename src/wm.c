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
        /* TODO: handle arrow keys, ctrl combos, etc. */
    }
}
