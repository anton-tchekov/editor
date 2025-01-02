CFLAGS = -std=c99 -pedantic -Wall -Wextra -Wshadow \
	-lSDL2 -lSDL2_ttf

all: debug

release:
	gcc main.c -o build/editor $(CFLAGS) -O3 -s -DNDEBUG
	x86_64-w64-mingw32-gcc main.c -o build/editor.exe -lmingw32 -lSDL2main $(CFLAGS) -mwindows -O3 -s -DNDEBUG

debug:
	gcc main.c -o editor $(CFLAGS) -g

run:
	./editor

clean:
	rm -f editor
