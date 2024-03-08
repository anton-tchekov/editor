/* Single line text input field */
typedef struct
{
	char *buf;
	u32 max, cursor, len;
} field;

static void field_reset(field *f)
{
	f->cursor = 0;
	f->len = 0;
	f->buf[0] = '\0';
}

static void field_left(field *f)
{
	if(f->cursor > 0)
	{
		--f->cursor;
	}
}

static void field_right(field *f)
{
	if(f->cursor < f->len)
	{
		++f->cursor;
	}
}

static void field_home(field *f)
{
	f->cursor = 0;
}

static void field_end(field *f)
{
	f->cursor = f->len;
}

static u32 field_backspace(field *f)
{
	if(f->cursor > 0)
	{
		char *p = f->buf + f->cursor;
		memmove(p - 1, p, f->len - f->cursor);
		--f->cursor;
		--f->len;
		return 1;
	}

	return 0;
}

static u32 field_delete(field *f)
{
	if(f->cursor < f->len)
	{
		char *p = f->buf + f->cursor;
		memmove(p, p + 1, f->len - f->cursor - 1);
		--f->len;
		return 1;
	}

	return 0;
}

static u32 field_char(field *f, u32 c)
{
	if(f->len >= f->max - 1)
	{
		return 0;
	}

	if(isprint(c))
	{
		char *p = f->buf + f->cursor;
		memmove(p + 1, p, f->len - f->cursor);
		f->buf[f->cursor++] = c;
		++f->len;
		return 1;
	}

	return 0;
}

static u32 field_key(field *f, u32 key, u32 c)
{
	u32 mod = 0;
	switch(key)
	{
	case KEY_LEFT:                  field_left(f);      break;
	case KEY_RIGHT:                 field_right(f);     break;
	case KEY_HOME:                  field_home(f);      break;
	case KEY_END:                   field_end(f);       break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:             mod = field_backspace(f); break;
	case KEY_DELETE:                mod = field_delete(f);    break;
	default:
		mod = field_char(f, c);
	}

	f->buf[f->len] = '\0';
	return mod;
}

static void field_render(field *f, u32 y, u32 focused, char *prompt)
{
	char *s;
	u32 x, i, c;

	for(s = prompt, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, screen_pack(c, ptp(PT_BG, PT_FG)));
	}

	s = f->buf;
	i = 0;
	if(focused)
	{
		for(; x < _screen_width; ++x, ++s, ++i)
		{
			screen_set(x, y, screen_pack((i < f->len) ? *s : ' ',
				(i == f->cursor) ? ptp(PT_FG, PT_BG) : ptp(PT_BG, PT_FG)));
		}
	}
	else
	{
		for(; x < _screen_width; ++x, ++s, ++i)
		{
			screen_set(x, y, screen_pack((i < f->len) ? *s : ' ',
				ptp(PT_BG, PT_FG)));
		}
	}
}
