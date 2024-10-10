#define GFX_INITIAL_WIDTH     480
#define GFX_INITIAL_HEIGHT    320
#define WINDOW_TITLE             "Editor"
#define DBL_CLICK_MS          500

#define COLOR_WHITE              0xFFFFFFFF

static u32 _quit;

static char *_font_name = "terminus.ttf";
static u32 _font_size = 16;
static u32 _char_width = 16, _char_height = 29;
static i32 _line_height = 35;
static u32 _gfx_width, _gfx_height;
static u32 _screen_width, _screen_height, _full_height;
static u32 _triple_click, _dbl_click;
static int _down;

static SDL_Texture *_font;
static SDL_Window *_window;
static SDL_Renderer *_renderer;

static void event_dblclick(u32 x, u32 y);
static void event_tripleclick(u32 x, u32 y);
static void event_mousemove(u32 x, u32 y);
static void event_mousedown(u32 x, u32 y);
static void event_shift_mousedown(u32 x, u32 y);
static void event_init(void);
static void event_key(u32 key, u32 chr);
static void event_render(void);
static void event_scroll(i32 y);
static u32 event_exit(void);

#define NUM_CHARS 128

static void update_dim(void)
{
	_screen_width = _gfx_width / _char_width;
	_full_height = _gfx_height / _line_height;
	_screen_height = _full_height - 1;
}

static void arrow(SDL_Surface *surface, int thick, int pos)
{
	u32 h = 1;
	int start = pos * _char_width;
	int x = _char_width - 2 * thick;
	for(; h < _char_height / 2; h += 2)
	{
		--x;
		SDL_Rect vline =
		{
			start + x,
			_char_height / 2 - h / 2,
			1,
			h
		};

		SDL_FillRect(surface, &vline, COLOR_WHITE);
	}

	SDL_Rect hline =
	{
		start,
		_char_height / 2 - thick / 2,
		x,
		thick
	};

	SDL_FillRect(surface, &hline, COLOR_WHITE);
}

static void circle(SDL_Surface *surface, int pos)
{
	int ox = pos * _char_width;
	int cx = _char_width / 2;
	int cy = _char_height / 2;
	int r = _char_width / 6;
	int r2 = r * r;
	for(u32 y = 0; y < _char_height; ++y)
	{
		int a = y - cy;
		int a2 = a * a;
		for(u32 x = 0; x < _char_width; ++x)
		{
			int b = x - cx;
			int b2 = b * b;
			if(a2 + b2 <= r2)
			{
				SDL_Rect rect = { ox + x, y, 1, 1 };
				SDL_FillRect(surface, &rect, COLOR_WHITE);
			}
		}
	}
}

static int font_load(char *font, i32 size)
{
	SDL_Surface *chars[NUM_CHARS];
	TTF_Font *ttf = TTF_OpenFont(font, size);
	if(!ttf)
	{
		return 1;
	}

	SDL_Color color = { 255, 255, 255, 0 };
	char letter[2];
	letter[1] = '\0';

	memset(chars, 0, sizeof(chars));

	for(u32 i = 33; i <= 126; ++i)
	{
		letter[0] = i;
		chars[i] = TTF_RenderText_Blended(ttf, letter, color);
	}

	for(u32 i = 0; i < ARRLEN(_extra_chars); ++i)
	{
		chars[_extra_chars[i].chr] =
			TTF_RenderUTF8_Blended(ttf, _extra_chars[i].utf8, color);
	}

	TTF_CloseFont(ttf);

	int max_h = 0, max_w = 0;
	for(u32 i = 0; i < NUM_CHARS; ++i)
	{
		SDL_Surface *s = chars[i];
		if(!s) { continue; }
		if(s->w > max_w)
		{
			max_w = s->w;
		}

		if(s->h > max_h)
		{
			max_h = s->h;
		}
	}

	_line_height = (((_line_height << 8) / _char_height) * max_h) >> 8;
	_char_width = max_w;
	_char_height = max_h;

	SDL_Surface *surface = SDL_CreateRGBSurface(0,
		_char_width * 16, _char_height * 8,
		32, 0xff, 0xff00, 0xff0000, 0xff000000);

	for(u32 i = 0; i < NUM_CHARS; ++i)
	{
		SDL_Surface *s = chars[i];
		if(!s) { continue; }
		SDL_Rect src = { 0, 0, s->w, s->h };
		int x = (i & 0x0F) * _char_width;
		int y = (i >> 4) * _char_height;
		SDL_Rect dst = { x, y, s->w, s->h };
		SDL_BlitSurface(s, &src, surface, &dst);
	}

	for(u32 i = 0; i < NUM_CHARS; ++i)
	{
		SDL_FreeSurface(chars[i]);
	}

	{
		SDL_Rect rect =
		{
			CHAR_BLOCK * _char_width,
			0,
			_char_width,
			_char_height
		};

		SDL_FillRect(surface, &rect, COLOR_WHITE);
	}

	int thick = _char_height / 16;
	if(thick < 1) { thick = 1; }

	/* Tab Start */
	SDL_Rect shline =
	{
		CHAR_TAB_START * _char_width + thick,
		_char_height / 2 - thick / 2,
		_char_width - thick,
		thick
	};

	SDL_FillRect(surface, &shline, COLOR_WHITE);

	int h = _char_height / 2;
	SDL_Rect vline =
	{
		CHAR_TAB_START * _char_width + thick,
		_char_height / 2 - h / 2,
		thick,
		h
	};

	SDL_FillRect(surface, &vline, COLOR_WHITE);

	/* Tab Middle */
	SDL_Rect mhline =
	{
		CHAR_TAB_MIDDLE * _char_width,
		_char_height / 2 - thick / 2,
		_char_width,
		thick
	};

	SDL_FillRect(surface, &mhline, COLOR_WHITE);

	/* Tab End */
	arrow(surface, thick, CHAR_TAB_END);

	/* Tab Both */
	arrow(surface, thick, CHAR_TAB_BOTH);

	/* Visible Space*/
	circle(surface, CHAR_VISIBLE_SPACE);

	/* Font */
	if(_font)
	{
		SDL_DestroyTexture(_font);
	}

	_font = SDL_CreateTextureFromSurface(_renderer, surface);
	_font_size = size;
	SDL_FreeSurface(surface);
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
	TTF_Quit();
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

static void fastrect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
	SDL_Rect blank = { 0, 0, _char_width, _char_height };
	SDL_Rect blank_dst = { x, y, w, h };

	SDL_SetTextureColorMod(_font,
		color_r(color), color_g(color), color_b(color));
	SDL_RenderCopy(_renderer, _font, &blank, &blank_dst);
}

static void render_char(u32 x, u32 y, u32 c, u32 fg, u32 bg)
{
	if(bg != COLOR_BG)
	{
		fastrect(x * _char_width, y * _line_height, _char_width, _line_height, bg);
	}

	if(c != ' ')
	{
		u32 pad = (_line_height - _char_height) / 2;
		SDL_Rect src = { (c & 0x0F) * _char_width, (c >> 4) * _char_height, _char_width, _char_height };
		SDL_Rect dst = { x * _char_width, y * _line_height + pad, _char_width, _char_height };

		SDL_SetTextureColorMod(_font,
			color_r(fg), color_g(fg), color_b(fg));
		SDL_RenderCopy(_renderer, _font, &src, &dst);
	}
}

static void render_cursor(u32 x, u32 y, u32 color)
{
	u32 thick = _char_height / 8;
	if(!thick) { thick = 1; }
	fastrect(x * _char_width, y * _line_height, thick, _line_height, color);
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
	else if(k == KEY_MINUS)                           { return CHAR_SZ; }
	else if(k == KEY_APOSTROPHE)                      { return CHAR_AE_LOWER; }
	else if(k == (KEY_APOSTROPHE | MOD_SHIFT))        { return CHAR_AE_UPPER; }
	else if(k == KEY_SEMICOLON)                       { return CHAR_OE_LOWER; }
	else if(k == (KEY_SEMICOLON | MOD_SHIFT))         { return CHAR_OE_UPPER; }
	else if(k == KEY_L_BRACKET)                       { return CHAR_UE_LOWER; }
	else if(k == (KEY_L_BRACKET | MOD_SHIFT))         { return CHAR_UE_UPPER; }
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
			else if(c == 'e') { return CHAR_EURO; }
			else if(c == 'm') { return CHAR_MICRO; }
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
			{ '!', '\"', CHAR_PARAGRAPH, '$', '%', '&', '/', '(', ')', '=' };

		static char numbers_altgr[] =
			{ 0, CHAR_POW2, CHAR_POW3, 0, 0, 0, '{', '[', ']', '}' };

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
		const u8 *state = SDL_GetKeyboardState(NULL);
		if(state[SDL_SCANCODE_LSHIFT] || state[SDL_SCANCODE_RSHIFT])
		{
			event_shift_mousedown(x, y);
		}
		else
		{
			event_mousedown(x, y);
		}
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
	size = clamp(size, 14, 64);
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

waitagain:
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

		case SDL_KEYUP:
			goto waitagain;

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
