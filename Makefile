all:
	gcc io.c -o editor -ansi -pedantic -Wall -Wextra -lSDL2 -O2 -g
#-DNDEBUG

run:
	./editor

valgrind:
	valgrind --leak-check=full --suppressions=memcheck.log ./editor

clean:
	rm -f editor
