#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>
#include <sys/time.h>
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include "types.h"
#include "keys.h"
#include "alloc.c"

static u32 umin(u32 a, u32 b)
{
	return a < b ? a : b;
}

static u32 umax(u32 a, u32 b)
{
	return a > b ? a : b;
}

#include "vector.c"
#include "file.c"
#include "colors.c"
#include "chars.c"
#include "gfx.c"
#include "editor.c"
