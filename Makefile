# Makefile for i3lock-next

PREFIX		?= /usr/local
DATAROOTDIR	?= /share

CC				?= gcc
## Manual CFLAGS override this Makefile
CFLAGS_DEFAULT  := $$(pkg-config --cflags fontconfig imlib2 x11 xrandr)       \
                   -Isrc/include                                              \
                   -std=gnu99 -O2 -Wall -Wextra -Wconversion                  \
                   -Wmissing-prototypes -Wmissing-declarations                \
                   -Wpointer-arith -Wwrite-strings -Wcast-qual                \
                   -Wbad-function-cast -Wformat-security -Wcast-align         \
                   -Wmissing-format-attribute -Winline -Wformat-nonliteral    \
                   -Wstrict-prototypes -fomit-frame-pointer                   \
                   -DPREFIX=\"$(PREFIX)\"
CFLAGS_DEFAULT  += $(CFLAGS)
CFLAGS          := $(CFLAGS_DEFAULT)

TARGET  := i3lock-next

SRC_DIR := ./src

BLD_DIR := ./build
OBJ_DIR := $(BLD_DIR)/obj
BIN_DIR	:= $(BLD_DIR)/bin

OBJ_LIST    := helpers.o main.o processing.o sanitizers.o
OBJ          = $(patsubst %, $(OBJ_DIR)/%, $(OBJ_LIST))

LIBS        := $$(pkg-config --libs fontconfig imlib2 x11 xrandr) -lm

## To keep lines <80 characters
NOTE    := Empty directories may exist if you had nothing installed in 
			
NOTE2   := Make sure to delete i3lock-next/i3lock-next.ini from your user 

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	$(info Compiling $<)
	@mkdir -p $(OBJ_DIR)
	@$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	$(info Linking $^)
	@$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS)
	$(info Build complete)

debug: CFLAGS += -Og -ggdb -fno-omit-frame-pointer -DDEBUG
debug: $(TARGET)

install:
	$(info Stripping unneeded symbols from binary)
	@strip --strip-unneeded -R .comment $(BIN_DIR)/$(TARGET)
	$(info Creating necessary directories)
	@install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	$(info Installing binary and data)
	@install -m 755 $(BIN_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@install -m 644 data/* $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)/
	$(info Install to $(DESTDIR)$(PREFIX) complete)

uninstall:
	$(info Removing binary and data directories)
	@rm $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@rm -r $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	$(info Uninstall complete)
	$(info $(NOTE)$(DESTDIR)$(PREFIX))
	$(info $(NOTE2).config directory)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	$(info Cleaned up $(OBJ_DIR) and $(BIN_DIR))

.PHONY: install uninstall debug clean

#vim: set noexpandtab :
