bindir ?= /usr/bin

ifeq ($(BOARD), mitx4)
	CFLAGS += -DBOARD_MITX4
endif
ifeq ($(BOARD), bn1bt1)
	CFLAGS += -DBOARD_BN1BT1
endif
REC_VERSION ?= 1.5
CFLAGS += -Wall -DRECOVERY -DDEBUG -DFRU_DEBUG -D_GNU_SOURCE
LDFLAGS = -lformw -lpanelw -lmenuw -lncursesw -ltinfo

FAKE_SHRED = 0x80000002
ifneq ($(BOARD),)
	CFLAGS += -DREAL_DEVICES
else
	CFLAGS += -DFAKE_SHRED=$(FAKE_SHRED)
endif

CFLAGS += -D_XOPEN_SOURCE_EXTENDED -D_FILE_OFFSET_BITS=64 -DREC_VERSION="$(REC_VERSION)"
CFLAFS += -D_GNU_SOURCE -D_DEFAULT_SOURCE
SOURCES = src/main.c src/top_menu.c src/main_page.c src/boot_page.c src/net_page.c src/recovery_page.c src/field_utils.c src/fru.c
OBJECTS = $(patsubst %.c, %.o, $(SOURCES))
PROJECT = recovery-ui

all: $(SOURCES) $(PROJECT)

$(PROJECT): $(OBJECTS)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

%.o : %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	-rm -f $(OBJECTS) $(PROJECT)

.PHONY: install
install:
	install -d -D -m 755 ${DESTDIR}${bindir}
	install -m 755 $(PROJECT) ${DESTDIR}${bindir}
	install -m 755 getrom.sh ${DESTDIR}${bindir}
	install -m 755 netconf.sh ${DESTDIR}${bindir}
