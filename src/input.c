/*
 * input.c — input event parsing
 */
#include "common.h"
#include <string.h>

/* Parse raw input bytes into Event structs.
 * Handles: ANSI escape sequences (arrow keys, mouse), plain text.
 */

int input_parse(const char *buf, int len, Event *out, int max_events)
{
    /* TODO: implement ANSI/xterm escape sequence parser */
    (void)buf;
    (void)len;
    (void)out;
    (void)max_events;
    return 0;
}
