ifndef PREFIX
  PREFIX = /usr/local
endif

ifndef DATAROOTDIR
  DATAROOTDIR = $(PREFIX)/share
endif

ifndef LIBEXECDIR
  LIBEXECDIR = $(PREFIX)/libexec
endif

CFLAGS = -std=gnu99 -O2 -Wall -lX11 -lImlib2 -lXrandr
LDFLAGS = $(CFLAGS)

OBJS := $(sort $(wildcard src/*.c))
OBJS := $(OBJS:.c=.o)

PROGRAM = i3lock-next-helper
_PROGRAM = i3lock-next

%.i: %.c
	$(CC) $(CFLAGS) -c -o $@ %<
	@echo " CC %<"

all: $(PROGRAM)

$(PROGRAM): ${OBJS}
	$(CC) $(LDFLAGS) -o $@ $^
	@echo " LD $@"
	strip --strip-unneeded $(PROGRAM)

clean:
	$(RM) src/*.o $(PROGRAM)

install: all
	install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	install -m 755 -d $(DESTDIR)$(LIBEXECDIR)/$(_PROGRAM)
	install -m 755 -d $(DESTDIR)$(DATAROOTDIR)/$(_PROGRAM)
	install -m 755 scripts/$(_PROGRAM) $(DESTDIR)$(PREFIX)/bin/$(_PROGRAM)
	install -m 755 $(PROGRAM) $(DESTDIR)$(LIBEXECDIR)/$(_PROGRAM)/$(PROGRAM)
	install -m 644 data/* $(DESTDIR)$(DATAROOTDIR)/$(_PROGRAM)/

install-doc:
	install -m 755 -d $(DESTDIR)$(DATAROOTDIR)/doc/$(_PROGRAM)
	install -m 644 doc/* $(DESTDIR)$(DATAROOTDIR)/doc/$(_PROGRAM)/

uninstall:
	$(RM) $(DESTDIR)$(PREFIX)/bin/$(_PROGRAM)
	$(RM) -r $(DESTDIR)$(LIBEXECDIR)/$(_PROGRAM)
	$(RM) -r $(DESTDIR)$(DATAROOTDIR)/$(_PROGRAM)

uninstall-doc:
	$(RM) -r $(DESTDIR)$(DATAROOTDIR)/doc/$(_PROGRAM)

.PHONY: clean uninstall
