CC := g++

CPPFLAGS := $(shell pkg-config --cflags gtk+-3.0 libcjson) -Wall -Wpedantic -Werror -g
LIBS := $(shell pkg-config --libs gtk+-3.0 libcjson) -lboost_filesystem
OBJECTS := build/filter.o build/graphics.o

all: build/log_viewer

build/log_viewer: ${OBJECTS}
	${CC} ${CPPFLAGS} src/log_viewer.cpp ${OBJECTS} -o $@ ${LIBS}

build/%.o: src/%.cpp
	mkdir -p build/
	${CC} ${CPPFLAGS} -c $< -o $@ ${LIBS}

clean:
	rm -rf build/

.PHONY: all clean
