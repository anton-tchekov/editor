CFLAGS = -std=c99 -pedantic -Wall -Wextra -Wshadow -lSDL2 -lSDL2_image

all: debug

release:
	gcc main.c -o editor $(CFLAGS) -O3 -s -DNDEBUG

debug:
	gcc main.c -o editor $(CFLAGS) -g

run:
	./editor

clean:
	rm -f editor

install:
	sudo cp editor /usr/bin/editor
