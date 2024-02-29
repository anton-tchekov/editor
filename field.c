/* Single line text input field */

typedef struct
{
	char *buf;
	u32 max, cursor, len;
} field;

static void field_add_nt(field *f)
{
	f->buf[f->len] = '\0';
}

static void field_reset(field *f)
{
	f->cursor = 0;
	f->len = 0;
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

static void field_backspace(field *f)
{
	if(f->cursor > 0)
	{
		char *p = f->buf + f->cursor;
		memmove(p - 1, p, f->len - f->cursor);
		--f->cursor;
		--f->len;
	}
}

static void field_delete(field *f)
{
	if(f->cursor < f->len)
	{
		char *p = f->buf + f->cursor;
		memmove(p, p + 1, f->len - f->cursor - 1);
		--f->len;
	}
}

static void field_char(field *f, u32 c)
{
	if(f->len >= f->max - 1)
	{
		return;
	}

	if(isprint(c))
	{
		char *p = f->buf + f->cursor;
		memmove(p + 1, p, f->len - f->cursor);
		f->buf[f->cursor++] = c;
		++f->len;
	}
}

static void field_key(field *f, u32 key, u32 c)
{
	switch(key)
	{
	case KEY_LEFT:                  field_left(f);      break;
	case KEY_RIGHT:                 field_right(f);     break;
	case KEY_HOME:                  field_home(f);      break;
	case KEY_END:                   field_end(f);       break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:             field_backspace(f); break;
	case KEY_DELETE:                field_delete(f);    break;
	default:                        field_char(f, c);   break;
	}
}
