#define _GNU_SOURCE

#include <SDL2/SDL.h>
#include <dirent.h>
#include <sys/types.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include <unistd.h>

#ifndef NDEBUG
#include <time.h>
#endif

#define TIMEKEY

#ifdef TIMEKEY
#include <sys/time.h>
#endif

#include "types.h"
#include "keys.h"
#include "terminus16.c"

#define CHAR_WIDTH              8
#define CHAR_HEIGHT            16
#define GFX_INITIAL_WIDTH     480
#define GFX_INITIAL_HEIGHT    320
#define WINDOW_TITLE             "Editor"
#define DBL_CLICK_MS          500

enum
{
	CHAR_VISIBLE_SPACE = 128,
	CHAR_TAB_START     = 129,
	CHAR_TAB_MIDDLE    = 130,
	CHAR_TAB_END       = 131,
	CHAR_TAB_BOTH      = 132,
	CHAR_MAX
};

enum
{
	COLOR_TABLE_BG,
	COLOR_TABLE_FG,
	COLOR_TABLE_GRAY,
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
	COLOR_TABLE_SELECTION,
	COLOR_TABLE_INFO,
	COLOR_TABLE_ERROR
};

static const u32 _color_table[] =
{
	0xFF1F1F1F, /* Background */
	0xFFFFFFFF, /* Foreground */
	0xFF777777, /* Gray / Line Number / Visible Space */
	0xFFFF6A55, /* Comment */
	0xFF9DFE88, /* Number */
	0xFFCE9178, /* String */
	0xFF569CD6, /* Type */
	0xFFC586C0, /* Keyword */
	0xFFFFD700, /* Parenthesis/Bracket/Brace Nesting Level 0 */
	0xFFDA70D6, /* Parenthesis/Bracket/Brace Nesting Level 1 */
	0xFF179FFF, /* Parenthesis/Bracket/Brace Nesting Level 2 */
	0xFFDCDCAA, /* Function Identifier */
	0xFF9CDCFE, /* Array Identifier */
	0xFF264F78, /* Selection */
	0xFF3C88CF, /* Info */
	0xFFFF0000, /* Error */
};

static u32 quit;

static u32 *_pixels;
static u32 _gfx_width, _gfx_height;

static u16 *_screen;
static u32 _screen_width, _screen_height;

static SDL_Texture *_framebuffer;
static SDL_Window *_window;
static SDL_Renderer *_renderer;

static void event_dblclick(u32 x, u32 y);
static void event_tripleclick(u32 x, u32 y);
static void event_mousemove(u32 x, u32 y);
static void event_mousedown(u32 x, u32 y);
static void event_init(void);
static void event_keyboard(u32 key, u32 chr, u32 state);
static void event_resize(void);
static void event_scroll(i32 y);
static u32 event_exit(void);

static void allocfail(void)
{
	fprintf(stderr, "Memory allocation failure\n");
	exit(1);
}

static u32 alloc_cnt, free_cnt;

static void *_malloc(size_t size)
{
	void *p = malloc(size);
	if(!p) { allocfail(); }
	++alloc_cnt;
	return p;
}

static void *_calloc(size_t num, size_t size)
{
	void *p = calloc(num, size);
	if(!p) { allocfail(); }
	++alloc_cnt;
	return p;
}

static void *_realloc(void *p, size_t size)
{
	if(!p)
	{
		++alloc_cnt;
	}

	p = realloc(p, size);
	if(!p) { allocfail(); }
	return p;
}

static void _free(void *p)
{
	if(p)
	{
		free(p);
		++free_cnt;
	}
}

#ifndef NDEBUG

static void print_mem(void)
{
	printf("%"PRIu32" allocs, %"PRIu32" frees\n", alloc_cnt, free_cnt);
}

#endif

static void resize_internal(u32 w, u32 h)
{
	_gfx_width = w;
	_gfx_height = h;
	_free(_pixels);
	_pixels = _calloc(w * h, sizeof(*_pixels));
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
	_free(_screen);
	_screen = _calloc(_screen_width * _screen_height, sizeof(*_screen));
}

static void resize(u32 w, u32 h)
{
	resize_internal(w, h);
	event_resize();
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
	_free(_screen);
	_free(_pixels);
	SDL_DestroyTexture(_framebuffer);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
#ifndef NDEBUG
	print_mem();
#endif
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

#if 0

/* Fill Rectangle Function is currently unused, but
 * will very likely be needed in the future */
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

#endif

static void glyph(u32 x, u32 y, u32 fg, u32 bg, u32 c)
{
	const u8 *start, *end;
	u32 *line;

	assert(x < _gfx_width);
	assert(y < _gfx_height);
	assert((x + CHAR_WIDTH) <= _gfx_width);
	assert((y + CHAR_HEIGHT) <= _gfx_height);
	/* assert(c >= 32 && c <= CHAR_MAX); */

	if(c < 32 || c > CHAR_MAX)
	{
		c = '?';
	}

	start = terminus16 + (c - 32) * CHAR_HEIGHT;
	end = start + CHAR_HEIGHT;
	line = _pixels + y * _gfx_width + x;
	for(; start < end; ++start)
	{
		c = *start;
		line[0] = (c & 0x80) ? fg : bg;
		line[1] = (c & 0x40) ? fg : bg;
		line[2] = (c & 0x20) ? fg : bg;
		line[3] = (c & 0x10) ? fg : bg;
		line[4] = (c & 0x08) ? fg : bg;
		line[5] = (c & 0x04) ? fg : bg;
		line[6] = (c & 0x02) ? fg : bg;
		line[7] = (c & 0x01) ? fg : bg;
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

static u32 screen_pack_set_bg(u32 v, u32 bg)
{
	v &= ~0xF0;
	return v | (bg << 4);
}

static u32 screen_pack_color_swap(u32 v)
{
	u32 a = v & 0x0F;
	u32 b = (v >> 4) & 0x0F;
	v &= ~0xFF;
	v |= (a << 4) | b;
	return v;
}

static void screen_set(u32 x, u32 y, u32 v)
{
	u16 *prev;

	assert(x < _screen_width);
	assert(y < _screen_height);

	prev = _screen + y * _screen_width + x;
	if(*prev != v)
	{
		*prev = v;
		glyph(x * CHAR_WIDTH, y * CHAR_HEIGHT,
			_color_table[v & 0x0F],
			_color_table[(v >> 4) & 0x0F], v >> 8);
	}
}

#include "vector.c"

#define FILE_CHUNK (16 * 1024)

enum
{
	FILE_READ_OK,
	FILE_READ_FAIL,
	FILE_READ_NOT_TEXT
};

static u32 is_text(const u8 *s, size_t len)
{
	u32 c;
	const u8 *end;
	for(end = s + len; s < end; ++s)
	{
		c = *s;
		if(!isprint(c) && c != '\n' && c != '\t')
		{
			return 0;
		}
	}

	return 1;
}

static u32 textfile_read(const char *filename, char **out)
{
	vector v;
	u32 rb;
	FILE *fp;
	if(!(fp = fopen(filename, "r")))
	{
		return FILE_READ_FAIL;
	}

	vector_init(&v, FILE_CHUNK);
	for(;;)
	{
		rb = fread((u8 *)v.data + v.len, 1, FILE_CHUNK, fp);
		if(!is_text((u8 *)v.data + v.len, rb))
		{
			vector_destroy(&v);
			fclose(fp);
			return FILE_READ_NOT_TEXT;
		}

		v.len += rb;
		if(rb < FILE_CHUNK)
		{
			break;
		}

		vector_reserve(&v, v.len + FILE_CHUNK);
	}

	if(ferror(fp))
	{
		vector_destroy(&v);
		fclose(fp);
		return FILE_READ_FAIL;
	}

	{
		u8 nt[1];
		nt[0] = '\0';
		vector_push(&v, 1, nt);
	}

	*out = v.data;
	fclose(fp);
	return FILE_READ_OK;
}

static u32 file_write(const char *filename, void *data, size_t len)
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

static u32 get_working_dir(char *buf)
{
	size_t len;
	getcwd(buf, PATH_MAX);
	len = strlen(buf);
	if(!len || (len > 0 && buf[len - 1] != '/'))
	{
		strcpy(buf + len, "/");
		++len;
	}

	return len;
}

static u32 file_exists(const char *fname)
{
	return access(fname, F_OK) == 0;
}

static u32 dir_iter(const char *path, void (*iter)(const char *, u32))
{
	DIR *dir;
	struct dirent *dp;

	if(!(dir = opendir(path)))
	{
		return 1;
	}

	while((dp = readdir(dir)))
	{
		iter(dp->d_name, dp->d_type == DT_DIR);
	}

	closedir(dir);
	return 0;
}

static int dir_sort_callback(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

static char **dir_sorted(const char *path, u32 *len)
{
	vector v;
	DIR *dir;
	struct dirent *dp;
	u32 i, count;
	char c[1];
	char *strs;
	char **ptrs;

	vector_init(&v, 1024);
	if(!(dir = opendir(path)))
	{
		return NULL;
	}

	count = 0;
	while((dp = readdir(dir)))
	{
		if(!strcmp(dp->d_name, "."))
		{
			continue;
		}

		vector_push(&v, strlen(dp->d_name), dp->d_name);
		if(dp->d_type == DT_DIR)
		{
			c[0] = '/';
			vector_push(&v, 1, c);
		}

		c[0] = '\0';
		vector_push(&v, 1, c);
		++count;
	}

	closedir(dir);
	vector_makespace(&v, 0, count * sizeof(char *));
	strs = (char *)v.data + count * sizeof(char *);
	ptrs = v.data;
	for(i = 0; i < count; ++i)
	{
		ptrs[i] = strs;
		strs += strlen(strs) + 1;
	}

	qsort(ptrs, count, sizeof(char *), dir_sort_callback);
	*len = count;
	return ptrs;
}

static void request_exit(void)
{
	quit = 1;
}

#ifndef NDEBUG

static int fuzzmain(void)
{
	/* CTRL is not included in modifiers to avoid file system corruption */
	static u32 rmods[] = { 0, KMOD_LSHIFT };
	SDL_Event e;
	init();
	event_init();
	srand(time(NULL));
	while(!quit)
	{
		SDL_UpdateTexture(_framebuffer, NULL, _pixels,
			_gfx_width * sizeof(u32));

		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _framebuffer, NULL, NULL);
		SDL_RenderPresent(_renderer);

		if(!SDL_PollEvent(&e))
		{
			e.type = SDL_KEYDOWN;
			e.key.keysym.scancode = rand() % KEY_COUNT;
			e.key.keysym.mod = rmods[rand() % (sizeof(rmods) / sizeof(*rmods))];
			e.key.repeat = 0;
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
			quit = event_exit();
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

#endif

#ifndef NDEBUG
int main(int argc, char *argv[])
#else
int main(void)
#endif
{
	u32 triple_click = 0, dbl_click = 0;
	int down = 0;
	SDL_Event e;

#ifndef NDEBUG
	if(argc == 2 && !strcmp(argv[1], "fuzz"))
	{
		return fuzzmain();
	}
#endif

	init();
	event_init();
	while(!quit)
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
			quit = event_exit();
			break;

		case SDL_KEYDOWN:
			{
				u32 key;
#ifdef TIMEKEY
				struct timeval tv;
				u64 t0, t1;
				gettimeofday(&tv, NULL);
				t0 = (u64)1000000 * (u64)tv.tv_sec + (u64)tv.tv_usec;
#endif
				key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key),
					e.key.repeat ? KEYSTATE_REPEAT : KEYSTATE_PRESSED);
#ifdef TIMEKEY
				gettimeofday(&tv, NULL);
				t1 = (u64)1000000 * (u64)tv.tv_sec + (u64)tv.tv_usec;
				printf("delta_t = %"PRIu64"\n", t1 - t0);
#endif
			}
			break;

		case SDL_KEYUP:
			{
				u32 key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key), KEYSTATE_RELEASED);
			}
			break;

		case SDL_MOUSEWHEEL:
			event_scroll(e.wheel.y);
			break;

		case SDL_MOUSEBUTTONDOWN:
			{
				u32 time;
				int x, y;
				down = 1;
				SDL_GetMouseState(&x, &y);
				time = SDL_GetTicks();
				x /= CHAR_WIDTH;
				y /= CHAR_HEIGHT;
				if(time < triple_click + DBL_CLICK_MS)
				{
					dbl_click = 0;
					triple_click = 0;
					event_tripleclick(x, y);
				}
				else if(time < dbl_click + DBL_CLICK_MS)
				{
					triple_click = dbl_click;
					dbl_click = 0;
					event_dblclick(x, y);
				}
				else
				{
					dbl_click = time;
					event_mousedown(x, y);
				}
			}
			break;

		case SDL_MOUSEBUTTONUP:
			down = 0;
			break;

		case SDL_MOUSEMOTION:
			{
				dbl_click = 0;
				triple_click = 0;
				if(down)
				{
					int x, y;
					SDL_GetMouseState(&x, &y);
					if(x < 0 || y < 0) { break; }
					event_mousemove(x / CHAR_WIDTH, y / CHAR_HEIGHT);
				}
			}
			break;
		}
	}

	destroy();
	return 0;
}

#include "editor.c"
