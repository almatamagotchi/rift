/*
 * pty.c — pseudo-terminal management
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Pty *pty_spawn(const char *cmd, int cols, int rows)
{
    /* TODO: openpty(), fork(), exec shell/cmd */
    (void)cmd;
    (void)cols;
    (void)rows;
    return NULL;
}

void pty_destroy(Pty *pty)
{
    if (!pty) return;
    if (pty->fd >= 0) close(pty->fd);
    /* TODO: kill/wait child */
    free(pty);
}

int pty_read(Pty *pty, char *out, size_t max)
{
    /* TODO: non-blocking read from pty master */
    (void)pty;
    (void)out;
    (void)max;
    return 0;
}

int pty_write(Pty *pty, const char *data, size_t len)
{
    /* TODO: write to pty master */
    (void)pty;
    (void)data;
    (void)len;
    return 0;
}

int pty_resize(Pty *pty, int cols, int rows)
{
    /* TODO: TIOCSWINSZ on pty master */
    (void)pty;
    (void)cols;
    (void)rows;
    return 0;
}
