# rift

A TUI window manager for the raw Linux/FreeBSD console — no X11, no Wayland.

**Status: scaffold**

### Features (planned)
- Mouse-driven draggable windows with box-drawn decorations
- Terminal apps inside (htop, vim, shell) via pseudo-terminals
- Snap tiling (left/right/top/bottom/corners/full)
- Taskbar with launcher and clock
- Cross-platform: Linux (GPM mouse) + FreeBSD (sysmouse)
- Pure C, no ncurses — direct console ioctl

### Build

```sh
# Linux
make platform=linux          # keyboard only
make platform=linux gpm      # with GPM mouse support

# FreeBSD
make platform=freebsd
```

### Architecture

```
src/
  wm.c          — main loop, window manager
  window.c/h    — window creation, z-order, focus
  render.c/h    — screen drawing, box decorations
  input.c/h     — ANSI escape + mouse event parser
  pty.c/h       — pseudo-terminal management
  bar.c/h       — taskbar (launcher, task buttons, clock)
  snap.c/h      — snapping / tiling
  common.h      — shared types and platform API

platform/
  linux.c       — raw tty + GPM mouse
  freebsd.c     — console ioctl + sysmouse
```

### Why "rift"?

A rift is a crack between worlds — this runs in the gap between raw console and full graphical environments. Also: it looks cool.
