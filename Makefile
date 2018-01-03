# Makefile for i3lock-next

## define PREFIX, DATAROOTDIR, and LIBDIR, if not defined
ifndef PREFIX
	PREFIX = /usr/local
endif

ifndef DATAROOTDIR
	DATAROOTDIR = /share
endif

ifndef LIBDIR
	LIBDIR = /libexec
endif

## define gcc as C compiler and set CFLAGS to warn about most things
CC = gcc
CFLAGS = -std=c11 -Wall -Wextra -O2 -fomit-frame-pointer
### note that -O2 is used since some warnings can only be detected by
### the data flow analysis performed at higher optimisation levels

## program name
TARGET = i3lock-next-helper
SCRIPT = i3lock-next

## define important directories for this project
### source files
SRC_DIR = ./src

### build files
BLD_DIR = ./build
OBJ_DIR = $(BLD_DIR)/obj
BIN_DIR = $(BLD_DIR)/bin

## list of object files that need to be compiled
OBJ_LIST = i3lock-next-helper.o
OBJ = $(patsubst %, $(OBJ_DIR)/%, $(OBJ_LIST))

## library linking flags
LIBS = -lX11 -lXrandr -lImlib2

## add PREFIX to CFLAGS (Thanks SuprDewd - see issue #4)
CFLAGS += -DPREFIX=\"$(PREFIX)\"

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@mkdir -p $(OBJ_DIR)
	@$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	@echo Linking $^
	@$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS)
	@echo Build complete

.PHONY: install uninstall debug warn clean

## build and install everything (including scripts/i3lock-next and data)
## (Thanks SuprDewd - see issue #4)
install:
	@echo Stripping unneeded symbols from binary
	@strip --strip-unneeded $(BIN_DIR)/$(TARGET)
	@install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(LIBDIR)/$(SCRIPT)
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(SCRIPT)
	@echo Installing script, binary, and data
	@install -m 755 scripts/$(SCRIPT) $(DESTDIR)$(PREFIX)/bin/$(SCRIPT)
	@install -m 755 $(BIN_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)$(LIBDIR)/$(SCRIPT)/$(TARGET)
	@install -m 644 data/* $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(SCRIPT)/
	@echo Replacing PREFIX in i3lock-next script
	@sed -i 's;PREFIX = ".*";PREFIX = "\$(PREFIX)";g' $(DESTDIR)$(PREFIX)/bin/$(SCRIPT)
	@echo Replacing LIBDIR in i3lock-next script
	@sed -i 's;LIBDIR = ".*";LIBDIR = "\$(LIBDIR)";g' $(DESTDIR)$(PREFIX)/bin/$(SCRIPT)
	@echo Install to $(DESTDIR)$(PREFIX) complete

## uninstall everything
uninstall:
	@echo Removing script
	@rm $(DESTDIR)$(PREFIX)/bin/$(SCRIPT)
	@echo Removing binary and data directories
	@rm -r $(DESTDIR)$(PREFIX)$(LIBDIR)/$(SCRIPT)
	@rm -r $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(SCRIPT)
	@echo Uninstall complete
	@echo NOTE: empty directories may exist if you had nothing installed in $(DESTDIR)$(PREFIX)
	@echo Make sure to delete i3lock-next.ini from your user\'s .config directory

## create a build for use with gdb
debug: CFLAGS += -ggdb -Werror -pedantic-errors -DDEBUG
debug: $(TARGET)

## create a build with all marginally useful warnings turned on
## -Werror is not included here as these can return false positives
warn: CFLAGS += -Wmissing-prototypes -Wmissing-declarations
warn: CFLAGS += -Wpointer-arith -Wwrite-strings -Wcast-qual
warn: CFLAGS += -Wbad-function-cast -Wformat-security -Wcast-align
warn: CFLAGS += -Wmissing-format-attribute -Winline
warn: CFLAGS += -Wformat-nonliteral -Wstrict-prototypes
warn: $(TARGET)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo Cleaned up $(OBJ_DIR) and $(BIN_DIR)
