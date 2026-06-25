# rift — TUI window manager for the raw console
# Build: make platform=(linux|freebsd)

CC      ?= cc
CFLAGS  ?= -O2 -g -Wall -Wextra -std=c11
LDFLAGS ?=

platform ?= linux

SRC_DIR  = src
PLAT_DIR = platform

SRCS  = $(SRC_DIR)/wm.c
SRCS += $(SRC_DIR)/window.c
SRCS += $(SRC_DIR)/render.c
SRCS += $(SRC_DIR)/input.c
SRCS += $(SRC_DIR)/pty.c
SRCS += $(SRC_DIR)/bar.c
SRCS += $(SRC_DIR)/snap.c
SRCS += $(SRC_DIR)/term.c

OBJS  = $(SRCS:.c=.o)

ifeq ($(platform),linux)
    SRCS  += $(PLAT_DIR)/linux.c
    CFLAGS += -DRIFT_PLATFORM_LINUX
    LDFLAGS += -lutil
    # GPM mouse support (optional)
    ifneq ($(findstring gpm,$(MAKECMDGOALS)),)
        CFLAGS += -DRIFT_GPM
        LDFLAGS += -lgpm
    endif
else ifeq ($(platform),freebsd)
    SRCS  += $(PLAT_DIR)/freebsd.c
    CFLAGS = -O2 -g -Wall -Wextra -std=gnu11 -DRIFT_PLATFORM_FREEBSD
else
    $(error Unknown platform: $(platform). Use make platform=linux or platform=freebsd)
endif

OBJS = $(patsubst %.c,%.o,$(SRCS))

TARGET = rift

.PHONY: all clean gpm

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) $(LDFLAGS)

$(SRC_DIR)/%.o: $(SRC_DIR)/%.c $(SRC_DIR)/common.h
	$(CC) $(CFLAGS) -c -o $@ $<

$(PLAT_DIR)/%.o: $(PLAT_DIR)/%.c $(SRC_DIR)/common.h
	$(CC) $(CFLAGS) -c -o $@ $<

gpm: CFLAGS += -DRIFT_GPM
gpm: LDFLAGS += -lgpm
gpm: $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
