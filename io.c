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
#include "terminus.c"

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
	PT_BG,
	PT_FG,
	PT_GRAY,
	PT_COMMENT,
	PT_NUMBER,
	PT_STRING,
	PT_TYPE,
	PT_KEYWORD,
	PT_BRACE,
	PT_BRACKET,
	PT_PAREN,
	PT_FN,
	PT_ARRAY,
	PT_SEL,
	PT_INFO,
	PT_ERROR
};

static u32 _color_table[] =
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

static u32 _quit;

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

static u32 _alloc_cnt, _free_cnt;

static void *_malloc(size_t size)
{
	void *p;

	p = malloc(size);
	if(!p) { allocfail(); }
	++_alloc_cnt;
	return p;
}

static void *_calloc(size_t num, size_t size)
{
	void *p;

	p = calloc(num, size);
	if(!p) { allocfail(); }
	++_alloc_cnt;
	return p;
}

static void *_realloc(void *p, size_t size)
{
	if(!p)
	{
		++_alloc_cnt;
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
		++_free_cnt;
	}
}

#ifndef NDEBUG

static void print_mem(void)
{
	printf("%"PRIu32" allocs, %"PRIu32" frees\n", _alloc_cnt, _free_cnt);
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
	u32 key;

	key = scancode;
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
	u32 nomods;

	nomods = k & 0xFF;
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
		u32 c;

		c = nomods - KEY_A + 'a';
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
		static char numbers[] =
			{ '1', '2', '3', '4', '5', '6', '7', '8', '9', '0' };

		static char numbers_shift[] =
			{ '!', '\"', 0, '$', '%', '&', '/', '(', ')', '=' };

		static char numbers_altgr[] =
			{ 0, 0, 0, 0, 0, 0, '{', '[', ']', '}' };

		u32 idx;

		idx = nomods - KEY_1;
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

static void glyph(u32 x, u32 y, u32 fg, u32 bg, u32 c)
{
	u8 *start, *end;
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

	start = _terminus + (c - 32) * CHAR_HEIGHT;
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

static void clipboard_store(char *str)
{
	SDL_SetClipboardText(str);
}

static char *clipboard_load(void)
{
	return SDL_GetClipboardText();
}

static u32 ptp(u32 fg, u32 bg)
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
	u32 a, b;

	a = v & 0x0F;
	b = (v >> 4) & 0x0F;
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

static u32 is_text(u8 *s, size_t len)
{
	u32 c;
	u8 *end;
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

static u32 textfile_read(char *filename, char **out)
{
	vec v;
	u32 rb;
	FILE *fp;
	if(!(fp = fopen(filename, "r")))
	{
		return FILE_READ_FAIL;
	}

	vec_init(&v, FILE_CHUNK);
	for(;;)
	{
		rb = fread((u8 *)v.data + v.len, 1, FILE_CHUNK, fp);
		if(!is_text((u8 *)v.data + v.len, rb))
		{
			vec_destroy(&v);
			fclose(fp);
			return FILE_READ_NOT_TEXT;
		}

		v.len += rb;
		if(rb < FILE_CHUNK)
		{
			break;
		}

		vec_reserve(&v, v.len + FILE_CHUNK);
	}

	if(ferror(fp))
	{
		vec_destroy(&v);
		fclose(fp);
		return FILE_READ_FAIL;
	}

	{
		u8 nt[1];
		nt[0] = '\0';
		vec_push(&v, 1, nt);
	}

	*out = v.data;
	fclose(fp);
	return FILE_READ_OK;
}

static u32 file_write(char *filename, void *data, size_t len)
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

static u32 file_exists(char *fname)
{
	return access(fname, F_OK) == 0;
}

static int dir_sort_callback(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

static char **dir_sorted(const char *path, u32 *len)
{
	vec v;
	DIR *dir;
	struct dirent *dp;
	u32 i, count;
	char c[1];
	char *strs;
	char **ptrs;

	vec_init(&v, 1024);
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

		vec_push(&v, strlen(dp->d_name), dp->d_name);
		if(dp->d_type == DT_DIR)
		{
			c[0] = '/';
			vec_push(&v, 1, c);
		}

		c[0] = '\0';
		vec_push(&v, 1, c);
		++count;
	}

	closedir(dir);
	vec_makespace(&v, 0, count * sizeof(char *));
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

static void io_runcmd(char *cmd, char *arg[])
{
	if(!fork())
	{
		execvp(cmd, arg);
	}
}

static void request_exit(void)
{
	_quit = 1;
}

static u64 get_ticks(void)
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (u64)1000000 * (u64)tv.tv_sec + (u64)tv.tv_usec;
}

#ifndef NDEBUG

static int fuzzmain(void)
{
	/* CTRL is not included in modifiers to avoid file system corruption */
	static u32 rmods[] = { 0, KMOD_LSHIFT };
	u64 t0, t1;
	u32 cnt;
	SDL_Event e;

	cnt = 0;
	init();
	event_init();
	srand(time(NULL));
	SDL_RenderSetVSync(_renderer, 0);
	while(!_quit)
	{
		t0 = get_ticks();
		if(t0 >= t1 + 1000000)
		{
			printf("cnt = %d\n", cnt);
			cnt = 0;
			t1 = t0;
		}

		SDL_UpdateTexture(_framebuffer, NULL, _pixels,
			_gfx_width * sizeof(u32));

		SDL_RenderClear(_renderer);
		SDL_RenderCopy(_renderer, _framebuffer, NULL, NULL);
		SDL_RenderPresent(_renderer);

		if(!SDL_PollEvent(&e))
		{
			e.type = SDL_KEYDOWN;
			e.key.keysym.scancode = rand() % KEY_COUNT;
			e.key.keysym.mod = rmods[rand() % ARRLEN(rmods)];
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
			_quit = event_exit();
			break;

		case SDL_KEYDOWN:
			{
				u32 key;

				key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key),
					e.key.repeat ? KEYSTATE_REPEAT : KEYSTATE_PRESSED);
			}
			break;

		case SDL_KEYUP:
			{
				u32 key;

				key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key), KEYSTATE_RELEASED);
			}
			break;
		}

		++cnt;
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
	u32 triple_click, dbl_click;
	int down;
	SDL_Event e;

#ifndef NDEBUG
	if(argc == 2 && !strcmp(argv[1], "fuzz"))
	{
		return fuzzmain();
	}
#endif

	down = 0;
	dbl_click = 0;
	triple_click = 0;
	init();
	event_init();
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
			_quit = event_exit();
			break;

		case SDL_KEYDOWN:
			{
				u32 key;
#ifdef TIMEKEY
				u64 t0, t1;
				t0 = get_ticks();
#endif
				key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_keyboard(key, key_to_chr(key),
					e.key.repeat ? KEYSTATE_REPEAT : KEYSTATE_PRESSED);
#ifdef TIMEKEY
				t1 = get_ticks();
				printf("delta_t = %"PRIu64"\n", t1 - t0);
#endif
			}
			break;

		case SDL_KEYUP:
			{
				u32 key;

				key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
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
