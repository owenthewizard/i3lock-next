# Makefile for i3lock-next

PREFIX		?= /usr/local
DATAROOTDIR	?= /share

CC				?= gcc
## Manual CFLAGS override this Makefile
CFLAGS_DEFAULT	:= -std=gnu99 -O2 -Wall -Wextra -Wconversion		  		  \
				   -Wmissing-prototypes -Wmissing-declarations				  \
				   -Wpointer-arith -Wwrite-strings -Wcast-qual				  \
				   -Wbad-function-cast -Wformat-security -Wcast-align		  \
				   -Wmissing-format-attribute -Winline -Wformat-nonliteral	  \
				   -Wstrict-prototypes -fomit-frame-pointer					  \
				   -DPREFIX=\"$(PREFIX)\"
CFLAGS_DEFAULT	+= $(CFLAGS)
CFLAGS			:= $(CFLAGS_DEFAULT)

TARGET	:= i3lock-next

SRC_DIR	:= ./src

BLD_DIR := ./build
OBJ_DIR := $(BLD_DIR)/obj
BIN_DIR	:= $(BLD_DIR)/bin

OBJ_LIST	:= i3lock-next.o
OBJ			= $(patsubst %, $(OBJ_DIR)/%, $(OBJ_LIST))

LIBS	:= $$(pkg-config --libs imlib2 x11 xrandr)

## To keep lines <80 characters
NOTE	:= 'NOTE: empty directories may exist if you had nothing installed in \
			$(DESTDIR)$(PREFIX)'
NOTE2	:= 'Make sure to delete i3lock-next/i3lock-next.ini from your user\'s \
			.config directory'

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.c
	@echo Compiling $<
	@mkdir -p $(OBJ_DIR)
	@$(CC) -c -o $@ $< $(CFLAGS) $(LIBS)

$(TARGET): $(OBJ)
	@mkdir -p $(BIN_DIR)
	@echo Linking $^
	@$(CC) -o $(BIN_DIR)/$@ $^ $(CFLAGS) $(LIBS)
	@echo Build complete

debug: CFLAGS += -Og -ggdb -fno-omit-frame-pointer -DDEBUG
debug: $(TARGET)

install:
	@echo Stripping unneeded symbols from binary
	@strip --strip-unneeded -R .comment $(BIN_DIR)/$(TARGET)
	@echo Creating necessary directories
	@install -m 755 -d $(DESTDIR)$(PREFIX)/bin
	@install -m 755 -d $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	@echo Installing binary and data
	@install -m 755 $(BIN_DIR)/$(TARGET) $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@install -m 644 data/* $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)/
	@echo Install to $(DESTDIR)$(PREFIX) complete

uninstall:
	@echo Removing binary and data directories
	@rm $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@rm -r $(DESTDIR)$(PREFIX)$(DATAROOTDIR)/$(TARGET)
	@echo Uninstall complete
	@echo $(NOTE)
	@echo $(NOTE2)

clean:
	@rm -rf $(OBJ_DIR) $(BIN_DIR)
	@echo Cleaned up $(OBJ_DIR) and $(BIN_DIR)

.PHONY: install uninstall debug clean

#vim: set noexpandtab :
