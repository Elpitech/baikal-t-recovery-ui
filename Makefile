CROSS_COMPILE ?=
CROSS_ROOT?=
PREFIX ?= .
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I$(CROSS_ROOT)/usr/include
LDFLAGS = -L$(CROSS_ROOT)/usr/lib -lpanelw -lncursesw -ltinfow -lmenuw
REAL_DEVICES ?= no
FAKE_SHRED = 0x80000002

ifeq ($(REAL_DEVICES),yes)
CFLAGS += -DREAL_DEVICES
else
CFLAGS += -DFAKE_SHRED=$(FAKE_SHRED)
endif

CFLAGS += -D_XOPEN_SOURCE_EXTENDED
CFLAFS += -D_GNU_SOURCE -D_DEFAULT_SOURCE -I/usr/include/ncursesw
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

.PHONY: install
install:
ifneq ($(PREFIX),.)
	cp $(PROJECT) $(PREFIX)
endif
