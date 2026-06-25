/*
 * pty.c — pseudo-terminal management
 */
#include "common.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>

#ifdef __linux__
#include <pty.h>       /* openpty */
#else
#include <libutil.h>   /* openpty on FreeBSD */
#endif

Pty *pty_spawn(const char *cmd, int cols, int rows)
{
    int master;
    pid_t pid;

    pid = forkpty(&master, NULL, NULL, NULL);
    if (pid < 0) return NULL;

    if (pid == 0) {
        /* child: set up terminal size and exec the command */
        struct winsize ws = { .ws_row = rows, .ws_col = cols,
                              .ws_xpixel = 0, .ws_ypixel = 0 };
        ioctl(STDIN_FILENO, TIOCSWINSZ, &ws);

        /* build argv: split cmd at spaces */
        char *sh_cmd = NULL;
        const char *shell = getenv("SHELL");
        if (!shell || !*shell) shell = "/bin/sh";

        if (cmd && *cmd) {
            sh_cmd = strdup(cmd);
            if (sh_cmd) {
                execl(shell, shell, "-c", sh_cmd, (char *)NULL);
                free(sh_cmd);
            }
        } else {
            execl(shell, shell, (char *)NULL);
        }

        _exit(127);
    }

    /* parent: configure master fd non-blocking */
    int flags = fcntl(master, F_GETFL, 0);
    if (flags >= 0) fcntl(master, F_SETFL, flags | O_NONBLOCK);

    Pty *pty = calloc(1, sizeof(Pty));
    if (!pty) { close(master); return NULL; }

    pty->fd  = master;
    pty->pid = pid;
    pty->buf_len = 0;

    return pty;
}

void pty_destroy(Pty *pty)
{
    if (!pty) return;
    if (pty->fd >= 0) {
        close(pty->fd);
        pty->fd = -1;
    }
    if (pty->pid > 0) {
        /* send SIGHUP, wait briefly, then SIGKILL if still alive */
        kill(pty->pid, SIGHUP);
        int status;
        pid_t w = waitpid(pty->pid, &status, WNOHANG);
        if (w == 0) {
            struct timespec ts = { .tv_sec = 0, .tv_nsec = 50000000 };
            nanosleep(&ts, NULL);
            w = waitpid(pty->pid, &status, WNOHANG);
            if (w == 0) {
                kill(pty->pid, SIGKILL);
                waitpid(pty->pid, &status, 0);
            }
        }
    }
    free(pty);
}

int pty_read(Pty *pty, char *out, size_t max)
{
    if (!pty || pty->fd < 0) return -1;

    /* drain existing buffer first */
    if (pty->buf_len > 0) {
        size_t n = (size_t)pty->buf_len < max ? (size_t)pty->buf_len : max;
        memcpy(out, pty->buf, n);
        pty->buf_len -= (int)n;
        if (pty->buf_len > 0)
            memmove(pty->buf, pty->buf + n, pty->buf_len);
        return (int)n;
    }

    ssize_t n = read(pty->fd, out, max);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
    return (int)n;
}

int pty_write(Pty *pty, const char *data, size_t len)
{
    if (!pty || pty->fd < 0) return -1;
    ssize_t n = write(pty->fd, data, len);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) return 0;
        return -1;
    }
    return (int)n;
}

int pty_resize(Pty *pty, int cols, int rows)
{
    if (!pty || pty->fd < 0) return -1;
    struct winsize ws = { .ws_row = rows, .ws_col = cols,
                          .ws_xpixel = 0, .ws_ypixel = 0 };
    return ioctl(pty->fd, TIOCSWINSZ, &ws);
}
