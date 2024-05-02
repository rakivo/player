CC = clang
CLIBS = -lm -lraylib -lplug
CFLAGS = -Wall -Wextra -Werror -pedantic -ggdb -fPIC
LDFLAGS = -shared

BIN = build/player
LIB_PLUG = build/libplug.so

.PHONY: clean all

all: $(BIN) $(LIB_PLUG)

$(BIN): main.c
	$(CC) $(CFLAGS) -o $@ main.c -L./build/ $(CLIBS)

$(LIB_PLUG): plug.c plug.h
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ plug.c

clean:
	rm -f $(LIB_PLUG) $(BIN)
