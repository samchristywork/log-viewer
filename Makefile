CC := g++

CPPFLAGS := $(shell pkg-config --cflags gtk+-3.0) -Wall -Wpedantic -Werror
LIBS := $(shell pkg-config --libs gtk+-3.0) -lboost_filesystem

all: build/log_viewer

build/log_viewer: log_viewer.cpp
	mkdir -p build/
	${CC} ${CPPFLAGS} log_viewer.cpp -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
