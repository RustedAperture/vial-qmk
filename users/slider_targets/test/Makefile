CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I..

.PHONY: all clean
all: test_core
	./test_core

test_core: test_core.c ../slider_targets_core.c ../slider_targets_core.h
	$(CC) $(CFLAGS) -o $@ test_core.c ../slider_targets_core.c

clean:
	rm -f test_core
