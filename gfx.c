#define GFX_INITIAL_WIDTH     480
#define GFX_INITIAL_HEIGHT    320
#define WINDOW_TITLE             "Editor"
#define DBL_CLICK_MS          500

static u32 _quit;

static u32 _char_width = 16, _char_height = 29;
static u32 _gfx_width, _gfx_height;
static u32 _screen_width, _screen_height;
static u32 _triple_click, _dbl_click;
static int _down;

static SDL_Texture *_font;
static SDL_Window *_window;
static SDL_Renderer *_renderer;

static void event_dblclick(u32 x, u32 y);
static void event_tripleclick(u32 x, u32 y);
static void event_mousemove(u32 x, u32 y);
static void event_mousedown(u32 x, u32 y);
static void event_init(void);
static void event_key(u32 key, u32 chr);
static void event_render(void);
static void event_scroll(i32 y);
static u32 event_exit(void);

static void resize(u32 w, u32 h)
{
	_gfx_width = w;
	_gfx_height = h;
	_screen_width = w / _char_width;
	_screen_height = h / _char_height;
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

	if(!(_font = IMG_LoadTexture(_renderer, "font_big.png")))
	{
		printf("Error creating SDL_Texture: %s\n",
			SDL_GetError());
		SDL_DestroyRenderer(_renderer);
		SDL_DestroyWindow(_window);
		SDL_Quit();
		exit(1);
	}

	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_ADD);
	resize(GFX_INITIAL_WIDTH, GFX_INITIAL_HEIGHT);
}

static void destroy(void)
{
	SDL_DestroyTexture(_font);
	SDL_DestroyRenderer(_renderer);
	SDL_DestroyWindow(_window);
	SDL_Quit();
	_alloc_report();
}

static u32 color_r(u32 color)
{
	return (color >> 16) & 0xFF;
}

static u32 color_g(u32 color)
{
	return (color >> 8) & 0xFF;
}

static u32 color_b(u32 color)
{
	return (color) & 0xFF;
}

static void render_char(u32 x, u32 y, u32 c, u32 fg, u32 bg)
{
	if(c < 32 || c > 126)
	{
		c = '?';
	}

	c -= 32;
	SDL_Rect src = { c * _char_width, 0, _char_width, _char_height };
	SDL_Rect dst = { x * _char_width, y * _char_height, _char_width, _char_height };

	SDL_Rect blank = { 1510, 0, _char_width, _char_height };

	SDL_SetTextureColorMod(_font,
		color_r(bg), color_g(bg), color_b(bg));
	SDL_RenderCopy(_renderer, _font, &blank, &dst);

	if(c != ' ')
	{
		SDL_SetTextureColorMod(_font,
			color_r(fg), color_g(fg), color_b(fg));
		SDL_RenderCopy(_renderer, _font, &src, &dst);
	}
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

static void clipboard_store(char *str)
{
	SDL_SetClipboardText(str);
}

static char *clipboard_load(void)
{
	return SDL_GetClipboardText();
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

static void handle_mouseup(void)
{
	_down = 0;
}

static void handle_mousedown(void)
{
#ifdef TIMEKEY
	u64 t0, t1;
	t0 = get_ticks();
#endif

	u32 time;
	int x, y;

	_down = 1;
	SDL_GetMouseState(&x, &y);
	time = SDL_GetTicks();
	x /= _char_width;
	y /= _char_height;
	if(time < _triple_click + DBL_CLICK_MS)
	{
		_dbl_click = 0;
		_triple_click = 0;
		event_tripleclick(x, y);
	}
	else if(time < _dbl_click + DBL_CLICK_MS)
	{
		_triple_click = _dbl_click;
		_dbl_click = 0;
		event_dblclick(x, y);
	}
	else
	{
		_dbl_click = time;
		event_mousedown(x, y);
	}

#ifdef TIMEKEY
	t1 = get_ticks();
	printf("MOUSEDOWN delta_t = %"PRIu64"\n", t1 - t0);
#endif
}

static void handle_mousemove(void)
{
#ifdef TIMEKEY
	u64 t0, t1;
	t0 = get_ticks();
#endif

	_dbl_click = 0;
	_triple_click = 0;
	if(_down)
	{
		int x, y;

		SDL_GetMouseState(&x, &y);
		if(x >= 0 && y >= 0)
		{
			event_mousemove(x / _char_width, y / _char_height);
		}
	}

#ifdef TIMEKEY
	t1 = get_ticks();
	printf("MOUSEMOVE delta_t = %"PRIu64"\n", t1 - t0);
#endif
}

static void handle_scroll(i32 y)
{
#ifdef TIMEKEY
	u64 t0, t1;
	t0 = get_ticks();
#endif
	event_scroll(y);
	handle_mousemove();
#ifdef TIMEKEY
	t1 = get_ticks();
	printf("SCROLL delta_t = %"PRIu64"\n", t1 - t0);
#endif
}

static void handle_render(void)
{
	SDL_SetRenderDrawColor(_renderer,
		color_r(COLOR_BG), color_g(COLOR_BG), color_b(COLOR_BG), 255);
	SDL_RenderClear(_renderer);
	event_render();
	SDL_RenderPresent(_renderer);
}

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

		handle_render();
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
				u32 key = convert_key(e.key.keysym.scancode, e.key.keysym.mod);
				event_key(key, key_to_chr(key));
			}
			break;
		}

		++cnt;
	}

	destroy();
	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Event e;

	if(argc == 2 && !strcmp(argv[1], "fuzz"))
	{
		return fuzzmain();
	}

	_down = 0;
	_dbl_click = 0;
	_triple_click = 0;
	init();
	event_init();
	while(!_quit)
	{
		{
#ifdef TIMEKEY
			u64 t0, t1;
			t0 = get_ticks();
#endif
			handle_render();
#ifdef TIMEKEY
			t1 = get_ticks();
			printf("RENDER delta_t = %"PRIu64"\n", t1 - t0);
#endif
		}

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
				event_key(key, key_to_chr(key));
#ifdef TIMEKEY
				t1 = get_ticks();
				printf("KEY delta_t = %"PRIu64"\n", t1 - t0);
#endif
			}
			break;

		case SDL_MOUSEWHEEL:
			handle_scroll(e.wheel.y);
			break;

		case SDL_MOUSEBUTTONDOWN:
			handle_mousedown();
			break;

		case SDL_MOUSEBUTTONUP:
			handle_mouseup();
			break;

		case SDL_MOUSEMOTION:
			handle_mousemove();
			break;
		}
	}

	destroy();
	return 0;
}
