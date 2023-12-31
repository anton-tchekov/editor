#define _GNU_SOURCE

#include <SDL2/SDL.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include "types.h"
#include "keys.h"
#include "Terminus16.c"
#include "Terminus16_Bold.c"

#define CHAR_WIDTH              8
#define CHAR_HEIGHT            16
#define GFX_INITIAL_WIDTH     480
#define GFX_INITIAL_HEIGHT    320
#define WINDOW_TITLE             "Editor"

enum
{
	CHAR_VISIBLE_SPACE = 128,
	CHAR_TAB_START     = 129,
	CHAR_TAB_MIDDLE    = 130,
	CHAR_TAB_END       = 131,
	CHAR_TAB_BOTH      = 132
};

enum
{
	COLOR_TABLE_BG,
	COLOR_TABLE_FG,
	COLOR_TABLE_LINE_NUMBER,
	COLOR_TABLE_COMMENT,
	COLOR_TABLE_NUMBER,
	COLOR_TABLE_STRING,
	COLOR_TABLE_TYPE,
	COLOR_TABLE_KEYWORD,
	COLOR_TABLE_BRACE,
	COLOR_TABLE_BRACKET,
	COLOR_TABLE_PAREN,
	COLOR_TABLE_FN,
	COLOR_TABLE_ARRAY,
	COLOR_TABLE_VISIBLE_SPACE,
	COLOR_TABLE_COMMENT_ASM,
	COLOR_TABLE_ERROR
};

static const u32 _color_table[] =
{
	0xFF000000, /* Background */
	0xFFFFFFFF, /* Foreground */
	0xFF777777, /* Line Number */
	0xFF6A9955, /* Comment */
	0xFFB5CEA8, /* Number */
	0xFFCE9178, /* String */
	0xFF569CD6, /* Type */
	0xFFC586C0, /* Keyword */
	0xFFFFD700, /* Parenthesis/Bracket/Brace Nesting Level 0 */
	0xFFDA70D6, /* Parenthesis/Bracket/Brace Nesting Level 1 */
	0xFF179FFF, /* Parenthesis/Bracket/Brace Nesting Level 2 */
	0xFFDCDCAA, /* Function Identifier */
	0xFF9CDCFE, /* Array Identifier */
	0xFF454545, /* Visible Space */
	0xFFD72424, /* Assembly Comment */
	0xFFFF0000, /* Error */
};

static u8 _quit;
static u32 *_pixels;
static u32 _gfx_width, _gfx_height;

static u16 *_screen;
static u32 _screen_width, _screen_height;

static SDL_Texture *_framebuffer;
static SDL_Window *_window;
static SDL_Renderer *_renderer;

static void event_init(int argc, char **argv, u32 w, u32 h);
static void event_keyboard(u32 key, u32 chr, u32 state);
static void event_resize(u32 w, u32 h);
static void event_exit(void);

static void resize_internal(u32 w, u32 h)
{
	_gfx_width = w;
	_gfx_height = h;
	if(_pixels)
	{
		free(_pixels);
	}

	_pixels = calloc(w * h, sizeof(*_pixels));
	if(_framebuffer)
	{
		SDL_DestroyTexture(_framebuffer);
	}

	_framebuffer = SDL_CreateTexture(_renderer,
		SDL_PIXELFORMAT_ARGB8888,
		SDL_TEXTUREACCESS_STREAMING,
		w, h);

	_screen_width = w / CHAR_WIDTH;
	_screen_height = h / CHAR_HEIGHT;
	if(_screen)
	{
		free(_screen);
	}

	_screen = calloc(_screen_width * _screen_height, sizeof(*_screen));
}

static void resize(u32 w, u32 h)
{
	resize_internal(w, h);
	event_resize(_screen_width, _screen_height);
}

static void init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Error initializing SDL; SDL_Init: %s\n",
			SDL_GetError());
		exit(1);
	}

	if(!(_window = SDL_CreateWindow(WINDOW_TITLE,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		GFX_INITIAL_WIDTH, GFX_INITIAL_HEIGHT,
		SDL_WINDOW_RESIZABLE)))
	{
		printf("Error creating SDL_Window: %s\n",
			SDL_GetError());
		SDL_Quit();
		exit(1);
	}

	if(!(_renderer = SDL_CreateRenderer
		(_window, -1, SDL_RENDERER_ACCELERATED)))
	{
		printf("Error creating SDL_Renderer: %s\n",
			SDL_GetError());
		SDL_DestroyWindow(_window);
		SDL_Quit();
		exit(1);
	}

	SDL_SetRenderDrawColor(_renderer, 255, 255, 255, 255);
	resize_internal(GFX_INITIAL_WIDTH, GFX_INITIAL_HEIGHT);
}

static void destroy(void)
{
	event_exit();
	free(_screen);
	free(_pixels);
	SDL_DestroyTexture(_framebuffer);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
}

static u32 convert_key(i32 scancode, i32 mod)
{
	u32 key = scancode;
	if(mod & (KMOD_LCTRL | KMOD_RCTRL))
	{
		key |= MOD_CTRL;
	}

	if(mod & KMOD_LALT)
	{
		key |= MOD_ALT;
	}

	if(mod & KMOD_RALT)
	{
		key |= MOD_ALT_GR;
	}

	if(mod & (KMOD_LGUI | KMOD_RGUI))
	{
		key |= MOD_OS;
	}

	if(mod & (KMOD_LSHIFT | KMOD_RSHIFT))
	{
		key |= MOD_SHIFT;
	}

	return key;
}

static u32 key_to_chr(u32 k)
{
	u32 nomods = k & 0xFF;

	if(nomods == KEY_TAB)                             { return '\t'; }
	else if(nomods == KEY_BACKSPACE)                  { return '\b'; }
	else if(nomods == KEY_RETURN)                     { return '\n'; }
	else if(nomods == KEY_SPACE)                      { return ' '; }
	else if(k == (KEY_COMMA | MOD_SHIFT))             { return ';'; }
	else if(k == (KEY_COMMA))                         { return ','; }
	else if(k == (KEY_PERIOD | MOD_SHIFT))            { return ':'; }
	else if(k == (KEY_PERIOD))                        { return '.'; }
	else if(k == (KEY_SLASH | MOD_SHIFT))             { return '_'; }
	else if(k == (KEY_SLASH))                         { return '-'; }
	else if(k == (KEY_BACKSLASH | MOD_SHIFT))         { return '\''; }
	else if(k == (KEY_BACKSLASH))                     { return '#'; }
	else if(k == (KEY_R_BRACKET | MOD_SHIFT))         { return '*'; }
	else if(k == (KEY_R_BRACKET | MOD_ALT_GR))        { return '~'; }
	else if(k == (KEY_R_BRACKET))                     { return '+'; }
	else if(k == (KEY_NON_US_BACKSLASH | MOD_SHIFT))  { return '>'; }
	else if(k == (KEY_NON_US_BACKSLASH | MOD_ALT_GR)) { return '|'; }
	else if(k == KEY_NON_US_BACKSLASH)                { return '<'; }
	else if(k == (KEY_MINUS | MOD_SHIFT))             { return '?'; }
	else if(k == (KEY_MINUS | MOD_ALT_GR))            { return '\\'; }
	else if(k == (KEY_EQUALS | MOD_SHIFT))            { return '`'; }
	else if(k == KEY_GRAVE)                           { return '^'; }
	else if(nomods >= KEY_A && nomods <= KEY_Z)
	{
		u32 c = nomods - KEY_A + 'a';

		if(c == 'z') { c = 'y'; }
		else if(c == 'y') { c = 'z'; }

		if(k & MOD_ALT_GR)
		{
			if(c == 'q') { return '@'; }
		}

		if(k & MOD_SHIFT)
		{
			c = toupper(c);
		}

		return c;
	}
	else if(nomods >= KEY_1 && nomods <= KEY_0)
	{
		static const char numbers[] =
			{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

		static const char numbers_shift[] =
			{ '!', '\"', 0, '$', '%', '&', '/', '(', ')', '=' };

		static const char numbers_altgr[] =
			{ 0, 0, 0, 0, 0, 0, '{', '[', ']', '}' };

		u32 idx = nomods - KEY_1;

		if(k & MOD_SHIFT)
		{
			return numbers_shift[idx];
		}
		else if(k & MOD_ALT_GR)
		{
			return numbers_altgr[idx];
		}
		else
		{
			return numbers[idx];
		}
	}

	return 0;
}

static void rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
	u32 x0;
	u32 *line;

	assert(x < _gfx_width);
	assert(y < _gfx_height);
	assert(w < _gfx_width);
	assert(h < _gfx_height);
	assert((x + w) <= _gfx_width);
	assert((y + h) <= _gfx_height);

	line = _pixels + y * _gfx_width + x;
	while(h)
	{
		for(x0 = 0; x0 < w; ++x0)
		{
			line[x0] = color;
		}

		line += _gfx_width;
		--h;
	}
}

static void glyph(u32 x, u32 y, u32 fg, u32 bg, u32 c)
{
	u32 w0, h0, mask;
	const u8 *start;
	u32 *line;

	assert(x < _gfx_width);
	assert(y < _gfx_height);
	assert((x + CHAR_WIDTH) <= _gfx_width);
	assert((y + CHAR_HEIGHT) <= _gfx_height);

	start = terminus16 + (c - 32) * CHAR_HEIGHT;
	line = _pixels + y * _gfx_width + x;
	for(h0 = 0; h0 < CHAR_HEIGHT; ++h0)
	{
		mask = (1 << (CHAR_WIDTH - 1));
		for(w0 = 0; w0 < CHAR_WIDTH; ++w0)
		{
			line[w0] = (start[h0] & mask) ? fg : bg;
			mask >>= 1;
		}

		line += _gfx_width;
	}
}

static void clipboard_store(const char *str)
{
	SDL_SetClipboardText(str);
}

static char *clipboard_load(void)
{
	return SDL_GetClipboardText();
}

static u32 screen_color(u32 fg, u32 bg)
{
	return fg | (bg << 4);
}

static u32 screen_pack(u32 c, u32 color)
{
	return color | (c << 8);
}

static u32 screen_pack_color_swap(u32 v)
{
	u32 a = v & 0x0F;
	u32 b = (v >> 4) & 0x0F;
	v &= ~0xFF;
	v |= (a << 4) | b;
	return v;
}

static void render_char(u32 x, u32 y, u32 c, u32 fg, u32 bg)
{
	glyph(x * CHAR_WIDTH, y * CHAR_HEIGHT,
		_color_table[fg], _color_table[bg], c);
}

static void screen_set(u32 x, u32 y, u32 c, u32 color)
{
	u16 *prev, new;

	assert(x < _screen_width);
	assert(y < _screen_height);

	prev = &_screen[y * _screen_width + x];
	new = color | (c << 8);
	if(*prev != new)
	{
		*prev = new;
		render_char(x, y, c, color & 0x0F, (color >> 4) & 0x0F);
	}
}

static void screen_set_pack(u32 x, u32 y, u32 v)
{
	screen_set(x, y, v >> 8, v & 0xFF);
}

#define READFILE_CHUNK 1024

static char *file_read(const char *filename, size_t *len)
{
	size_t size, cap, rb;
	char *buf;
	FILE *fp;
	if(!(fp = fopen(filename, "r")))
	{
		return NULL;
	}

	cap = READFILE_CHUNK;
	if(!(buf = malloc(cap)))
	{
		goto cleanup_file;
	}

	size = 0;
	while((rb = fread(buf + size, 1, READFILE_CHUNK, fp)) == READFILE_CHUNK)
	{
		size += rb;
		if(size + READFILE_CHUNK > cap)
		{
			char *nd;
			cap <<= 1;
			if(!(nd = realloc(buf, cap)))
			{
				goto cleanup_alloc;
			}

			buf = nd;
		}
	}

	size += rb;
	if(ferror(fp))
	{
		goto cleanup_alloc;
	}

	fclose(fp);
	*len = size;
	return buf;

cleanup_alloc:
	free(buf);

cleanup_file:
	fclose(fp);
	return NULL;
}

static int file_write(const char *filename, void *data, size_t len)
{
	FILE *fp;
	if(!(fp = fopen(filename, "w")))
	{
		return 1;
	}

	if(fwrite(data, 1, len, fp) != len)
	{
		return 1;
	}

	fclose(fp);
	return 0;
}

static int dir_iter(const char *path, void *data,
	void (*iter)(void *, const char *, int))
{
	DIR *dir;
	struct dirent *dp;

	if(!(dir = opendir(path)))
	{
		return 1;
	}

	while((dp = readdir(dir)))
	{
		iter(data, dp->d_name, dp->d_type == DT_DIR);
	}

	closedir(dir);
	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Event e;
	init();
	event_init(argc, argv, _screen_width, _screen_height);
	while(!_quit)
	{
		SDL_UpdateTexture(_framebuffer, NULL, _pixels,
			_gfx_width * sizeof(u32));

		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _framebuffer, NULL, NULL);
		SDL_RenderPresent(_renderer);

		if(!SDL_WaitEvent(&e))
		{
			break;
		}

		switch(e.type)
		{
		case SDL_WINDOWEVENT:
			if(e.window.event == SDL_WINDOWEVENT_RESIZED)
			{
				resize(e.window.data1, e.window.data2);
			}
			break;

		case SDL_QUIT:
			_quit = 1;
			break;

		case SDL_KEYDOWN:
			{
				u32 key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key),
					e.key.repeat ? KEYSTATE_REPEAT : KEYSTATE_PRESSED);
			}
			break;

		case SDL_KEYUP:
			{
				u32 key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key), KEYSTATE_RELEASED);
			}
			break;
		}
	}

	destroy();
	return 0;
}

#include "editor.c"
