CROSS_COMPILE ?=
CROSS_ROOT?=
PREFIX ?= .
CC = $(CROSS_COMPILE)gcc
CFLAGS = -Wall -I$(CROSS_ROOT)/usr/include -DRECOVERY -DDEBUG
#LDFLAGS = -L$(CROSS_ROOT)/usr/lib -lpanelw -lncursesw -ltinfow -lmenuw
REAL_DEVICES ?= no
FAKE_SHRED = 0x80000002
REC_VERSION=$(shell python gsuf/gsuf.py --main-branch master)

ifeq ($(REAL_DEVICES),yes)
CFLAGS += -DREAL_DEVICES
LDFLAGS = -L$(CROSS_ROOT)/usr/lib -lformw -lpanelw -lmenuw -lncursesw -ltinfow
else
CFLAGS += -DFAKE_SHRED=$(FAKE_SHRED)
LDFLAGS = -L$(CROSS_ROOT)/lib/x86_64-linux-gnu/ -lformw -lpanelw -lmenuw -lncursesw
endif

CFLAGS += -D_XOPEN_SOURCE_EXTENDED -D_FILE_OFFSET_BITS=64 -DREC_VERSION="$(REC_VERSION)"
CFLAFS += -D_GNU_SOURCE -D_DEFAULT_SOURCE -I/usr/include/ncursesw
SOURCES = src/main.c src/top_menu.c src/main_page.c src/boot_page.c src/net_page.c src/recovery_page.c src/fru.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
PROJECT = recovery-ui

all: $(SOURCES) $(PROJECT)

prepare:
	if [ ! -e gsuf ]; then git clone https://github.com/snegovick/gsuf.git; fi

$(PROJECT): prepare $(OBJECTS)
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
