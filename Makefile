all:
	gcc io.c -o editor -ansi -pedantic -Wall -Wextra -lSDL2 -O2 -g
#-DNDEBUG

run:
	./editor

clean:
	rm -f editor
