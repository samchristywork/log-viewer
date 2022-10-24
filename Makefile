CC := g++

CPPFLAGS := $(shell pkg-config --cflags gtk+-3.0 libcjson) -Wall -Wpedantic -Werror -g
LIBS := $(shell pkg-config --libs gtk+-3.0 libcjson) -lboost_filesystem

all: build/log_viewer

build/graphics.o: src/graphics.cpp src/graphics.hpp
	mkdir -p build/
	${CC} ${CPPFLAGS} src/graphics.cpp -c -o $@ ${LIBS}

build/filter.o: src/filter.cpp src/filter.hpp
	mkdir -p build/
	${CC} ${CPPFLAGS} src/filter.cpp -c -o $@ ${LIBS}

build/log_viewer: src/log_viewer.cpp src/log_viewer.hpp build/graphics.o build/filter.o
	mkdir -p build/
	${CC} ${CPPFLAGS} src/log_viewer.cpp build/graphics.o build/filter.o -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: clean
