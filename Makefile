CC = clang

CLIBS = -lm -lraylib
CFLAGS = -Wall -Wextra -Werror -pedantic -fPIC -ferror-limit=100
LDFLAGS = -shared

BIN = build/out
PLUG_BIN = build/plug
PLUG_OUT = build/libplug.so

.PHONY: clean

all: $(BIN) $(PLUGS) plug_bin_clean

$(BIN): src/main.c $(PLUG_OUT)
	$(CC) $(CFLAGS) $(CLIBS) -o $@ src/main.c

$(PLUG_OUT): src/plug.c src/plug.h
	$(CC) $(CFLAGS) $(CLIBS) $(LDFLAGS) -o $@ src/plug.c

plug_bin_clean:
	rm -f $(PLUG_BIN)

clean:
	rm -f $(PLUG_OUT) $(BIN) plug_bin_clean
