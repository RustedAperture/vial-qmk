CC ?= gcc
CFLAGS = -std=c11 -Wall -Wextra -Werror -I..

.PHONY: all clean
all: test_core test_calibrated
	./test_core
	./test_calibrated

test_core: test_core.c ../slider_targets_core.c ../slider_targets_core.h
	$(CC) $(CFLAGS) -o $@ test_core.c ../slider_targets_core.c

# Same core built with the numpad keymap's calibrated ADC range
test_calibrated: test_calibrated.c ../slider_targets_core.c ../slider_targets_core.h
	$(CC) $(CFLAGS) -DST_ADC_MIN=16 -DST_ADC_MAX=940 -o $@ test_calibrated.c ../slider_targets_core.c

clean:
	rm -f test_core test_calibrated
