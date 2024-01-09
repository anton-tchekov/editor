all:
	gcc io.c -o editor -ansi -pedantic -Wall -Wextra -lSDL2 -O2 -DNDEBUG

clean:
	rm -f editor
