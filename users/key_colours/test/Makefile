CC ?= gcc
CFLAGS = -std=c11 -DKEY_COLOURS_LAYERS=2 -DKEY_COLOURS_LEDS=14 -DKEY_COLOURS_BALANCE_G=140 -DKEY_COLOURS_BALANCE_B=95 -Wall -Wextra -Werror -I..

.PHONY: all clean
all: test_core
	./test_core

test_core: test_core.c ../key_colours_core.c ../key_colours_core.h
	$(CC) $(CFLAGS) -o $@ test_core.c ../key_colours_core.c

clean:
	rm -f test_core
