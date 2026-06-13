/*
 * input.c — input event parsing
 * Parses raw terminal input bytes into Event structs.
 * Handles: plain text, control characters, ANSI/xterm escape sequences
 * including CSI, SS3, SGR mouse, urxvt mouse, and x10 mouse.
 */
#include "common.h"
#include <string.h>
#include <stdlib.h>

/* ---- helpers for parsing escape-sequence parameters ---- */

/* parse up to 8 semicolon-separated integers from buf[pos..end),
 * stopping at a non-digit, non-';' byte.  Returns number of params
 * parsed and sets *term to the final byte (the terminator character). */
static int parse_csi_params(const char *buf, int start, int len,
                            int params[8], char *term)
{
    int n = 0;
    int i = start;
    int cur = -1;
    *term = 0;
    while (i < len && n < 8) {
        char c = buf[i];
        if (c >= '0' && c <= '9') {
            if (cur < 0) cur = 0;
            cur = cur * 10 + (c - '0');
            i++;
            continue;
        }
        if (c == ';') {
            params[n++] = (cur < 0) ? 0 : cur;
            cur = -1;
            i++;
            continue;
        }
        /* terminator */
        params[n++] = (cur < 0) ? 0 : cur;
        *term = c;
        return n;
    }
    if (n < 8 && cur >= 0)
        params[n++] = cur;
    if (i < len) *term = buf[i];
    return n;
}

/* ---- CSI final-byte dispatch (cursor keys, etc.) ---- */

static int csi_to_key(char term, int params[8], int nparam, int *mod)
{
    *mod = 0;

    /* modifier-encoded CSI: e.g. \033[1;5A = ctrl+up.
     * The first param is the base code (1=up,...) and the second
     * param encodes modifiers (2=shift, 3=alt, 4=shift+alt,
     * 5=ctrl, 6=shift+ctrl, 7=alt+ctrl, 8=shift+alt+ctrl). */
    if (nparam >= 2 && term == '~') {
        int base  = params[0];
        int mod_raw = params[1];
        if (mod_raw >= 2) {
            if (mod_raw & 1) *mod |= MOD_SHIFT;
            if (mod_raw & 2) *mod |= MOD_ALT;
            if (mod_raw & 4) *mod |= MOD_CTRL;
            if (mod_raw & 8) *mod |= MOD_META;
        }
        switch (base) {
            case 2:  return K_INS;
            case 3:  return K_DEL;
            case 5:  return K_PGUP;
            case 6:  return K_PGDN;
            case 11: return K_F1;
            case 12: return K_F2;
            case 13: return K_F3;
            case 14: return K_F4;
            case 15: return K_F5;
            default: break;
        }
        /* F5+ also come as 15-34 */
        if (base >= 15 && base <= 34)
            return K_F1 + (base - 11);
        return -1;
    }

    /* modifier-encoded cursor keys: \033[1;2A etc. */
    if (nparam >= 2 && (term >= 'A' && term <= 'D')) {
        int mod_raw = params[1];
        if (mod_raw >= 2) {
            if (mod_raw & 1) *mod |= MOD_SHIFT;
            if (mod_raw & 2) *mod |= MOD_ALT;
            if (mod_raw & 4) *mod |= MOD_CTRL;
            if (mod_raw & 8) *mod |= MOD_META;
        }
    }

    switch (term) {
        case 'A': return K_UP;
        case 'B': return K_DOWN;
        case 'C': return K_RIGHT;
        case 'D': return K_LEFT;
        case 'H': return K_HOME;
        case 'F': return K_END;
        case 'Z': return K_TAB;  /* shift-tab */
        default:  return -1;
    }
}

/* ---- SS3 (ESC O ...) dispatch ---- */

static int ss3_to_key(char term)
{
    switch (term) {
        case 'P': return K_F1;
        case 'Q': return K_F2;
        case 'R': return K_F3;
        case 'S': return K_F4;
        case 'A': return K_UP;
        case 'B': return K_DOWN;
        case 'C': return K_RIGHT;
        case 'D': return K_LEFT;
        case 'H': return K_HOME;
        case 'F': return K_END;
        default:  return -1;
    }
}

/* ---- mouse sequence parsing ---- */

enum { MS_X10 = 0, MS_SGR, MS_URXVT, MS_SGR_PIXEL };

/* parse mouse params from a CSI sequence whose terminator is 'M' or 'm' */
static int parse_mouse(char term, int params[8], int nparam,
                       Event *ev, int *mod)
{
    if (nparam < 3) return -1;
    int btn, col, row;

    /* SGR: \033[<btn;col;row M/m */
    if (term == 'M' || term == 'm') {
        btn = params[0];
        col = params[1];
        row = params[2];
        ev->mouse.btn = btn & 3;
        /* SGR wheel is encoded in bits 6-7 (64=wheel-up, 65=wheel-down) */
        if (btn >= 64) {
            ev->type = (btn & 1) ? EV_MOUSE_WHEEL_DOWN : EV_MOUSE_WHEEL_UP;
        } else {
            ev->type = (term == 'm') ? EV_MOUSE_UP : EV_MOUSE_DOWN;
        }
        ev->mouse.x = col;
        ev->mouse.y = row;
        *mod = 0;
        if (btn & 4)  *mod |= MOD_SHIFT;
        if (btn & 8)  *mod |= MOD_ALT;
        if (btn & 16) *mod |= MOD_CTRL;
        return 0;
    }

    /* urxvt-style: \033[btn;col;rowM (no '<' prefix, 1015 mode) */
    if (term == 'M' && nparam >= 3 && params[0] < 64) {
        btn = params[0];
        col = params[1];
        row = params[2];
        ev->mouse.btn = btn & 3;
        ev->type = EV_MOUSE_DOWN;
        ev->mouse.x = col;
        ev->mouse.y = row;
        *mod = 0;
        if (btn & 4)  *mod |= MOD_SHIFT;
        if (btn & 8)  *mod |= MOD_ALT;
        if (btn & 16) *mod |= MOD_CTRL;
        return 0;
    }

    return -1;
}

/* ---- main parser ---- */

int input_parse(const char *buf, int len, Event *out, int max_events)
{
    int nout = 0;
    int pos = 0;

    while (pos < len && nout < max_events) {
        unsigned char c = (unsigned char)buf[pos];

        /* ---- plain text ---- */
        if (c >= 32 && c != 127) {
            /* coalesce consecutive text bytes into a single key event */
            if (nout > 0 && out[nout - 1].type == EV_KEY &&
                out[nout - 1].key.key > 0 && out[nout - 1].key.key < 256 &&
                out[nout - 1].key.mod == 0) {
                pos++;
                continue;
            }
            if (nout >= max_events) break;
            out[nout].type = EV_KEY;
            out[nout].key.key = c;
            out[nout].key.mod = 0;
            nout++;
            pos++;
            continue;
        }

        /* ---- control characters ---- */
        if (c == '\n' || c == '\r') {
            if (nout >= max_events) break;
            out[nout].type = EV_KEY;
            out[nout].key.key = K_ENTER;
            out[nout].key.mod = 0;
            nout++;
            /* skip possible CR/LF pair */
            if (c == '\r' && pos + 1 < len && buf[pos + 1] == '\n')
                pos++;
            pos++;
            continue;
        }
        if (c == '\t') {
            if (nout >= max_events) break;
            out[nout].type = EV_KEY;
            out[nout].key.key = K_TAB;
            out[nout].key.mod = 0;
            nout++;
            pos++;
            continue;
        }
        if (c == 8 || c == 127) {  /* BS or DEL */
            if (nout >= max_events) break;
            out[nout].type = EV_KEY;
            out[nout].key.key = K_BSP;
            out[nout].key.mod = 0;
            nout++;
            pos++;
            continue;
        }

        /* ---- escape sequences ---- */
        if (c != 27) { pos++; continue; }  /* unknown control, skip */

        if (pos + 1 >= len) break;  /* incomplete sequence at buffer end */

        char c2 = buf[pos + 1];

        /* ESC ESC = literal ESC key */
        if (c2 == 27) {
            if (nout >= max_events) break;
            out[nout].type = EV_KEY;
            out[nout].key.key = K_ESC;
            out[nout].key.mod = 0;
            nout++;
            pos += 2;
            continue;
        }

        /* CSI: ESC [ ... */
        if (c2 == '[') {
            int start = pos + 2;
            int params[8];
            char term;
            int nparam = parse_csi_params(buf, start, len, params, &term);
            int seq_end = start;
            while (seq_end < len && buf[seq_end] != term) seq_end++;
            if (seq_end >= len) break;  /* incomplete */
            seq_end++;  /* consume terminator */

            if (nout >= max_events) break;

            /* SGR mouse: \033[<... */
            if (start < len && buf[start] == '<') {
                int mod = 0;
                if (parse_mouse(term, params, nparam, &out[nout], &mod) == 0) {
                    out[nout].key.mod = mod;
                    nout++;
                    pos = seq_end;
                    continue;
                }
            }
            /* urxvt mouse: \033[btn;col;rowM (no '<', in 1015 mode) */
            if (nparam >= 3 && params[0] < 64 && term == 'M') {
                int mod = 0;
                if (parse_mouse(term, params, nparam, &out[nout], &mod) == 0) {
                    out[nout].key.mod = mod;
                    nout++;
                    pos = seq_end;
                    continue;
                }
            }

            /* key dispatch */
            int mod = 0;
            int key = csi_to_key(term, params, nparam, &mod);
            if (key >= 0) {
                out[nout].type = EV_KEY;
                out[nout].key.key = key;
                out[nout].key.mod = mod;
                nout++;
                pos = seq_end;
                continue;
            }
            /* unrecognised CSI — consume and ignore */
            pos = seq_end;
            continue;
        }

        /* SS3: ESC O ... */
        if (c2 == 'O') {
            if (pos + 2 >= len) break;
            char term = buf[pos + 2];
            int key = ss3_to_key(term);
            if (key >= 0) {
                if (nout >= max_events) break;
                out[nout].type = EV_KEY;
                out[nout].key.key = key;
                out[nout].key.mod = 0;
                nout++;
                pos += 3;
                continue;
            }
            pos += 3;
            continue;
        }

        /* OSC (ESC ]), DCS (ESC P), SOS (ESC X), PM (ESC ^), APC (ESC _):
         * ignore the entire string-terminated sequence (ST = ESC \) */
        if (c2 == ']' || c2 == 'P' || c2 == 'X' || c2 == '^' || c2 == '_') {
            pos += 2;
            while (pos < len) {
                if (buf[pos] == '\033' && pos + 1 < len && buf[pos + 1] == '\\') {
                    pos += 2;
                    break;
                }
                if (buf[pos] == '\007') { pos++; break; }  /* BEL terminator */
                pos++;
            }
            continue;
        }

        /* ESC followed by unknown char: treat as plain ESC key */
        if (nout >= max_events) break;
        out[nout].type = EV_KEY;
        out[nout].key.key = K_ESC;
        out[nout].key.mod = 0;
        nout++;
        pos++;
    }

    return nout;
}
