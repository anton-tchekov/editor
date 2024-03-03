CFLAGS = -ansi -pedantic -Wall -Wextra -Wshadow -lSDL2

all: debug

release:
	gcc io.c -o editor $(CFLAGS) -O2 -s -DNDEBUG

debug:
	gcc io.c -o editor $(CFLAGS) -g

run:
	./editor

clean:
	rm -f editor

install:
	sudo cp editor /usr/bin/editor
