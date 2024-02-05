all:
	gcc io.c -o editor -ansi -pedantic -Wall -Wextra -lSDL2 -O2 -g
#-DNDEBUG

clean:
	rm -f editor
