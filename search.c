/* SR (Search and Replace) */
#define SR_COUNT 2

enum
{
	SR_IN_DIR      = 0x01,
	SR_REPLACE     = 0x02,
	SR_MATCH_CASE  = 0x04,
	SR_WHOLE_WORDS = 0x08,
	SR_USE_ESCSEQ  = 0x10
};

static tf _sr_tf_search, _sr_tf_replace;
static u8 _sr_flags, _sr_focus;

static u32 escape_seq(char *out, char *s)
{
	u32 c;

	for(; (c = *s); ++s)
	{
		if(c == '\\')
		{
			++s;
			c = *s;
			switch(c)
			{
			case 't':  c = '\t'; break;
			case 'n':  c = '\n'; break;
			case '\\': c = '\\'; break;
			case '\"': c = '\"'; break;
			case '\'': c = '\''; break;
			default: return 1;
			}
		}

		*out++ = c;
	}

	*out = '\0';
	return 0;
}

static void mode_search(void)
{
	_mode = MODE_SEARCH;
	_sr_flags &= ~(SR_REPLACE | SR_IN_DIR);
}

static void mode_search_in_dir(void)
{
	_mode = MODE_SEARCH;
	_sr_flags = (_sr_flags & ~SR_REPLACE) | SR_IN_DIR;
}

static void mode_replace(void)
{
	_mode = MODE_SEARCH;
	_sr_flags = (_sr_flags & ~SR_IN_DIR) | SR_REPLACE;
}

static void mode_replace_in_dir(void)
{
	_mode = MODE_SEARCH;
	_sr_flags |= SR_REPLACE | SR_IN_DIR;
}

static u32 sr_render(void)
{
	tf_render(&_sr_tf_search, 0, _sr_focus == 0, "Search: ");
	tf_render(&_sr_tf_replace, 1, _sr_focus == 1, "Replace: ");
	return 2;
}

static void sr_init(void)
{
	tf_init(&_sr_tf_search);
	tf_init(&_sr_tf_replace);
}

static void sr_destroy(void)
{
	tf_destroy(&_sr_tf_search);
	tf_destroy(&_sr_tf_replace);
}

static void sr_key(u32 key, u32 chr)
{
	switch(key)
	{
	case KEY_UP:
	case MOD_SHIFT | KEY_TAB:
		_sr_focus = dec_wrap(_sr_focus, SR_COUNT);
		break;

	case KEY_DOWN:
	case KEY_TAB:
		_sr_focus = inc_wrap(_sr_focus, SR_COUNT);
		break;

	case KEY_ESCAPE:
		mode_default();
		break;

	case MOD_CTRL | KEY_F:
		mode_search();
		break;

	case MOD_CTRL | MOD_SHIFT | KEY_F:
		mode_search_in_dir();
		break;

	case MOD_CTRL | KEY_H:
		mode_replace();
		break;

	case MOD_CTRL | MOD_SHIFT | KEY_H:
		mode_replace_in_dir();
		break;

	default:
		switch(_sr_focus)
		{
		case 0:
			tf_key(&_sr_tf_search, key, chr);
			break;

		case 1:
			tf_key(&_sr_tf_replace, key, chr);
			break;
		}
		break;
	}
}
