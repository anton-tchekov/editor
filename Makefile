CFLAGS = -ansi -pedantic -Wall -Wextra -lSDL2

all: debug

release:
	gcc io.c -o editor $(CFLAGS) -O2 -s -DNDEBUG

debug:
	gcc io.c -o editor $(CFLAGS) -g

run:
	./editor

clean:
	rm -f editor
