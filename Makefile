all:
	gcc io.c -o editor -ansi -pedantic -Wall -Wextra -lSDL2 -O2

clean:
	rm -f editor
