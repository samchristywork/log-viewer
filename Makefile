CC := g++

CPPFLAGS := $(shell pkg-config --cflags gtk+-3.0) -Wall -Wpedantic -Werror -g
LIBS := $(shell pkg-config --libs gtk+-3.0) -lboost_filesystem

all: build/log_viewer

build/graphics.o: graphics.cpp graphics.hpp
	mkdir -p build/
	${CC} ${CPPFLAGS} graphics.cpp -c -o $@ ${LIBS}

build/log_viewer: log_viewer.cpp log_viewer.hpp build/graphics.o
	mkdir -p build/
	${CC} ${CPPFLAGS} log_viewer.cpp build/graphics.o -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
