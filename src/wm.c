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

static void sig_handler(int sig)
{
    (void)sig;
    running = 0;
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
        int n = plat_read_input(inbuf, sizeof(inbuf));
        if (n > 0) {
            /* TODO: parse input into Events, dispatch to wm_handle_event */
            (void)n;
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
    /* TODO: create initial shell window, init bar */
    (void)cols;
    (void)rows;
    return 0;
}

void wm_shutdown(void)
{
    /* TODO: destroy all windows */
}

void wm_handle_event(const Event *ev)
{
    /* TODO: dispatch events */
    (void)ev;
}
