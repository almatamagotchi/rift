#ifndef RIFT_SNAP_H
#define RIFT_SNAP_H

#include "common.h"

typedef enum {
    SNAP_NONE,
    SNAP_LEFT,
    SNAP_RIGHT,
    SNAP_TOP,
    SNAP_BOTTOM,
    SNAP_TL,
    SNAP_TR,
    SNAP_BL,
    SNAP_BR,
    SNAP_FULL
} SnapDir;

void snap_apply(Window *win, SnapDir dir, int cols, int rows, int bar_h);

#endif /* RIFT_SNAP_H */
