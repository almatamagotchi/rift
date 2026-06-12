#ifndef RIFT_PTY_H
#define RIFT_PTY_H

#include "common.h"
#include <sys/types.h>

#define PTY_BUF_SZ 4096

struct Pty {
    int   fd;             /* master fd */
    pid_t pid;            /* child process */
    char  buf[PTY_BUF_SZ];
    int   buf_len;
};

Pty *pty_spawn(const char *cmd, int cols, int rows);
void pty_destroy(Pty *pty);
int  pty_read(Pty *pty, char *out, size_t max);
int  pty_write(Pty *pty, const char *data, size_t len);
int  pty_resize(Pty *pty, int cols, int rows);

#endif /* RIFT_PTY_H */
