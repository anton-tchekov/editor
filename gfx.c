#define GFX_INITIAL_WIDTH     480
#define GFX_INITIAL_HEIGHT    320
#define WINDOW_TITLE             "Editor"
#define DBL_CLICK_MS          500

static u32 _quit;

static char *_font_name = "terminus.ttf";
static u32 _font_size = 16;
static u32 _char_width = 16, _char_height = 29;
static i32 _line_height = 35;
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

#define NUM_CHARS 95

static void update_dim(void)
{
	_screen_width = _gfx_width / _char_width;
	_screen_height = _gfx_height / _line_height;
}

int font_load(char *font, i32 size)
{
	char chars[128];

	/* Load new ttf font */
	TTF_Font *ttf = TTF_OpenFont(font, size);
	if(!ttf)
	{
		return 1;
	}

    SDL_Color color = { 255, 255, 255, 0 };

	for(u32 i = 0; i < NUM_CHARS; ++i)
	{
		chars[i] = i + 32;
	}

	SDL_Surface *surface = TTF_RenderText_Blended(ttf, chars, color);

	/* Recalculate line height as ratio of font height */
	_line_height = (((_line_height << 8) / _char_height) * surface->h) >> 8;

	/* Only works with monospace fonts */
	_char_width = surface->w / NUM_CHARS;
	_char_height = surface->h;

	SDL_Surface *new_surface = SDL_CreateRGBSurface(0,
		surface->w + _char_width, surface->h,
		32, 0xff, 0xff00, 0xff0000, 0xff000000);

	SDL_Rect src = { 0, 0, surface->w, surface->h };
	SDL_Rect dst = { _char_width, 0, surface->w, surface->h };
	SDL_BlitSurface(surface, &src, new_surface, &dst);

	SDL_Rect rect = { 0, 0, _char_width, _char_height };
	SDL_FillRect(new_surface, &rect, 0xFFFFFFFF);

	/* Free previous font texture */
	if(_font)
	{
		SDL_DestroyTexture(_font);
	}

	_font = SDL_CreateTextureFromSurface(_renderer, new_surface);
	_font_size = size;
	SDL_FreeSurface(surface);
	SDL_FreeSurface(new_surface);
	update_dim();
	return 0;
}

static void resize(u32 w, u32 h)
{
	_gfx_width = w;
	_gfx_height = h;
	update_dim();
}

static void init(void)
{
	if(SDL_Init(SDL_INIT_VIDEO) < 0)
	{
		printf("Error initializing SDL; SDL_Init: %s\n", SDL_GetError());
		goto fail_sdl_init;
	}

	if(!(_window = SDL_CreateWindow(WINDOW_TITLE,
		SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
		GFX_INITIAL_WIDTH, GFX_INITIAL_HEIGHT,
		SDL_WINDOW_RESIZABLE)))
	{
		printf("Error creating SDL_Window: %s\n", SDL_GetError());
		goto fail_create_window;
	}

	if(!(_renderer = SDL_CreateRenderer
		(_window, -1, SDL_RENDERER_ACCELERATED)))
	{
		printf("Error creating SDL_Renderer: %s\n", SDL_GetError());
		goto fail_create_renderer;
	}

	if(TTF_Init())
	{
		printf("Loading initializing TTF: %s\n", TTF_GetError());
		goto fail_ttf_init;
	}

	if(font_load(_font_name, _font_size))
	{
		printf("Loading font failed: %s\n", SDL_GetError());
		goto fail_font_load;
	}

	SDL_SetRenderDrawBlendMode(_renderer, SDL_BLENDMODE_BLEND);
	resize(GFX_INITIAL_WIDTH, GFX_INITIAL_HEIGHT);
	return;

fail_font_load:
	TTF_Quit();
fail_ttf_init:
	SDL_DestroyRenderer(_renderer);
fail_create_renderer:
	SDL_DestroyWindow(_window);
fail_create_window:
	SDL_Quit();
fail_sdl_init:
	exit(1);
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

	if(bg != COLOR_BG)
	{
		SDL_Rect blank = { 0, 0, _char_width, _char_height };
		SDL_Rect blank_dst = { x * _char_width, y * _line_height, _char_width, _line_height };

		SDL_SetTextureColorMod(_font,
			color_r(bg), color_g(bg), color_b(bg));
		SDL_RenderCopy(_renderer, _font, &blank, &blank_dst);
	}

	if(c != ' ')
	{
		c -= 32;
		u32 pad = (_line_height - _char_height) / 2;
		SDL_Rect src = { (c + 1) * _char_width, 0, _char_width, _char_height };
		SDL_Rect dst = { x * _char_width, y * _line_height + pad, _char_width, _char_height };

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
	y /= _line_height;
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
			event_mousemove(x / _char_width, y / _line_height);
		}
	}

#ifdef TIMEKEY
	t1 = get_ticks();
	printf("MOUSEMOVE delta_t = %"PRIu64"\n", t1 - t0);
#endif
}

static i32 clamp(i32 val, i32 min, i32 max)
{
	if(val < min) { val = min; }
	if(val > max) { val = max; }
	return val;
}

static void handle_line_height(i32 y)
{
	_line_height += y;
	_line_height = clamp(_line_height, _char_height, 3 * _char_height / 2);
	update_dim();
}

static void handle_zoom(i32 y)
{
	i32 size = _font_size + y;
	size = clamp(size, 12, 64);
	font_load(_font_name, size);
}

static void handle_scroll(i32 y)
{
#ifdef TIMEKEY
	u64 t0, t1;
	t0 = get_ticks();
#endif
	const uint8_t *state = SDL_GetKeyboardState(NULL);
	if(state[SDL_SCANCODE_LCTRL] || state[SDL_SCANCODE_RCTRL])
	{
		if(state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])
		{
			handle_line_height(y);
		}
		else
		{
			handle_zoom(y);
		}
	}
	else
	{
		event_scroll(y);
		handle_mousemove();
	}

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
	u64 t0 = 0, t1 = 0;
	u32 cnt = 0;
	SDL_Event e;

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
