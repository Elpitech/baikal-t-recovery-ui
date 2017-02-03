CROSS_COMPILE?=
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall
LDFLAGS = -lpanelw $(shell pkg-config ncursesw --libs)
REAL_DEVICES ?= no
FAKE_ID = 0x80000002

ifeq ($(REAL_DEVICES),yes)
CFLAGS += -DREAL_DEVICES
else
CFLAGS += -DFAKE_ID=$(FAKE_ID)
endif

CFLAGS += -D_XOPEN_SOURCE_EXTENDED
CFLAFS += $(shell pkg-config ncursesw --cflags)
SOURCES = src/main.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
PROJECT = recovery-ui

all: $(SOURCES) $(PROJECT)

$(PROJECT): $(OBJECTS) 
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	rm $(OBJECTS) $(PROJECT)
