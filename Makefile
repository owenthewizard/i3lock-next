# Makefile for i3lock-next

PREFIX		?= /usr/local
DATAROOTDIR	?= /share

CC          ?= gcc

SRC_DIR     := ./src
## Manual CFLAGS override this Makefile
CFLAGS_DEFAULT  := $$(pkg-config --cflags fontconfig imlib2 x11 xrandr)       \
                   -I$(SRC_DIR)/include                                       \
                   -std=gnu99 -O2 -Wall -Wextra -Wconversion -Wshadow         \
                   -Wmissing-prototypes -Wmissing-declarations                \
                   -Wpointer-arith -Wwrite-strings -Wcast-qual                \
                   -Wbad-function-cast -Wformat-security -Wcast-align         \
                   -Wmissing-format-attribute -Winline -Wformat-nonliteral    \
                   -Wstrict-prototypes -fomit-frame-pointer                   \
                   -DPREFIX=\"$(PREFIX)\"
CFLAGS_DEFAULT  += $(CFLAGS)
CFLAGS          := $(CFLAGS_DEFAULT)

TARGET  := i3lock-next

BLD_DIR := ./build
OBJ_DIR := $(BLD_DIR)/obj
BIN_DIR	:= $(BLD_DIR)/bin

OBJ_LIST    := helpers.o main.o processing.o sanitizers.o
OBJ          = $(patsubst %, $(OBJ_DIR)/%, $(OBJ_LIST))

LIBS        := $$(pkg-config --libs fontconfig imlib2 x11 xrandr) -lm

## To keep lines <80 characters
NOTE    := Empty directories may exist if you had nothing installed in 
NOTE2   := Make sure to delete i3lock-next/i3lock-next.ini from your user 
MAN_N   := generate a fancy screenshot and call i3lock

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(info Linking $^)
	@$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS)
	$(info Build complete)

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c prepare
	$(info Compiling $<)
	@mkdir -p $(OBJ_DIR)
	@$(CC) -c -o $@ $< $(CFLAGS)

prepare:
	@$(shell ./replace_yucc_defaults.sh $(TARGET))
	@$(shell sed -i 's;PREFIX;\$(PREFIX);g' $(SRC_DIR)/include/$(TARGET).yucc)

debug: CFLAGS += -Og -ggdb -fno-omit-frame-pointer -DDEBUG
debug: $(TARGET)

genman:
	$(info Generating manpage)
	@mkdir -p $(BLD_DIR)/man
	@help2man -N -n "$(MAN_N)" $(BIN_DIR)/$(TARGET) > $(BLD_DIR)/man/$(TARGET).1

install:
	$(info Stripping unneeded symbols from binary)
	@strip --strip-unneeded -R .comment $(BIN_DIR)/$(TARGET)
	$(info Creating necessary directories)
	@install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(DATARROTDIR)/man/man1
	$(info Installing binary and data)
	@install -m 755 $(BIN_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@install -m 644 data/* $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)/
	$(info Installing man file)
	@install -m 644 $(BLD_DIR)/man/$(TARGET).1 $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/man/man1/
	$(info Install to $(DESTDIR)$(PREFIX) complete)

uninstall:
	$(info Removing binary and data directories)
	@rm $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@rm -r $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	$(info Removing man file)
	@rm $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/man/man1/$(TARGET).1
	$(info Uninstall complete)
	$(info $(NOTE)$(DESTDIR)$(PREFIX))
	$(info $(NOTE2).config directory)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR) $(BLD_DIR)/man
	@rm $(SRC_DIR)/include/$(TARGET).yucc
	$(info Cleaned up $(OBJ_DIR) and $(BIN_DIR) as well as \
		$(SRC_DIR)/include/$(TARGET).yucc)

.PHONY: install uninstall debug clean prepare

#vim: set noexpandtab :
