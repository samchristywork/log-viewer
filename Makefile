CC := gcc

CFLAGS := $(shell pkg-config --cflags gtk+-3.0) -Wall -Wpedantic -Werror
LIBS := $(shell pkg-config --libs gtk+-3.0)

all: build/log_viewer

build/log_viewer: log_viewer.c
	mkdir -p build/
	${CC} ${CFLAGS} log_viewer.c -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
