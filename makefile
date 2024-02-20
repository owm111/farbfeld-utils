CFLAGS = -g -Os -std=c99 -Wall -Wextra -Wpedantic
CPPFLAGS = -D_XOPEN_SOURCE=700
PREFIX ?= /usr/share
BINDIR ?= $(PREFIX)/bin
MANDIR ?= $(PREFIX)/man

all: rectangles extract

clean:
	$(RM) rectangles extract

install: rectangles extract
	install -m755 -D -t $(BINDIR) rectangles extract

uninstall:
	$(RM) $(BINDIR)/rectangles $(BINDIR)/extract

.PHONY: all clean install uninstall
