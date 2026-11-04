CC ?= gcc
CFLAGS ?= -std=c11 -O2 -Wall -Wextra -Wpedantic
.PHONY: all test clean
all: monitor
monitor: monitor.c
	$(CC) $(CFLAGS) -o $@ $<
test: monitor
	python3 -m unittest discover -s tests -v
clean:
	rm -f monitor
