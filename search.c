/* SR (Search and Replace) */
enum
{
	SR_INP_MATCH_CASE,
	SR_INP_WHOLE_WORDS,
	SR_INP_USE_ESCSEQ,
	SR_INP_CHK_END = SR_INP_USE_ESCSEQ,

	SR_INP_SEARCH,
	SR_INP_REPLACE,
	SR_COUNT
};

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

static void sr_open(void)
{
	_mode = MODE_SEARCH;
	_sr_focus = SR_INP_SEARCH;
}

static void mode_search(void)
{
	if(!_tb) { return; }
	sr_open();
	_sr_flags &= ~(SR_REPLACE | SR_IN_DIR);
}

static void mode_search_in_dir(void)
{
	sr_open();
	_sr_flags = (_sr_flags & ~SR_REPLACE) | SR_IN_DIR;
}

static void mode_replace(void)
{
	if(!_tb) { return; }
	sr_open();
	_sr_flags = (_sr_flags & ~SR_IN_DIR) | SR_REPLACE;
}

static void mode_replace_in_dir(void)
{
	sr_open();
	_sr_flags |= SR_REPLACE | SR_IN_DIR;
}

static void sr_title_render(void)
{
	char buf[4096 + 256];
	char *action, *print;

	print = action = (_sr_flags & SR_REPLACE) ? "Replace" : "Search";
	if(_sr_flags & SR_IN_DIR)
	{
		snprintf(buf, sizeof(buf), "%s in directory %s", action, _path_buf);
		print = buf;
	}

	ed_render_line_str(print, 0, 0, COLOR_FG, COLOR_INFO);
}

static void sr_put_chk(char *s, u32 idx, u32 mask)
{
	s[idx] = (_sr_flags & mask) ? 'X' : ' ';
}

#define SR_CHK_POS_MATCH_CASE    1
#define SR_CHK_POS_WHOLE_WORDS  17
#define SR_CHK_POS_USE_ESCSEQ   34

static u32 sr_chk_cur_sel(void)
{
	switch(_sr_focus)
	{
	case SR_INP_MATCH_CASE:
		return SR_CHK_POS_MATCH_CASE;

	case SR_INP_WHOLE_WORDS:
		return SR_CHK_POS_WHOLE_WORDS;

	case SR_INP_USE_ESCSEQ:
		return SR_CHK_POS_USE_ESCSEQ;
	}

	return _screen_width;
}

static void sr_chk_render(void)
{
	static char opt[] =
		"[ ] Match Case  [ ] Whole Words  [ ] Use Escape Sequences";

	u32 x, hl;
	char *s;

	sr_put_chk(opt, SR_CHK_POS_MATCH_CASE, SR_MATCH_CASE);
	sr_put_chk(opt, SR_CHK_POS_WHOLE_WORDS, SR_WHOLE_WORDS);
	sr_put_chk(opt, SR_CHK_POS_USE_ESCSEQ, SR_USE_ESCSEQ);

	x = 0;
	hl = sr_chk_cur_sel();
	for(s = opt; *s && x < _screen_width; ++s, ++x)
	{
		u32 fg = COLOR_BG;
		u32 bg = COLOR_FG;
		if(x == hl)
		{
			fg = COLOR_FG;
			bg = COLOR_BG;
		}

		render_char(x, 1, *s, fg, bg);
	}

	for(; x < _screen_width; ++x)
	{
		render_char(x, 1, ' ', COLOR_BG, COLOR_FG);
	}
}

static u32 sr_render(void)
{
	sr_title_render();
	sr_chk_render();
	tf_render(&_sr_tf_search, 2, _sr_focus == SR_INP_SEARCH, "Search: ");
	if(_sr_flags & SR_REPLACE)
	{
		tf_render(&_sr_tf_replace, 3, _sr_focus == SR_INP_REPLACE, "Replace: ");
		return 4;
	}

	return 3;
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

static char *sr_tf_esc(tf *t, u32 *out_len)
{
	char *in = tf_str(t);
	if(_sr_flags & SR_INP_USE_ESCSEQ)
	{
		/* Escaped string is never longer */
		char *out = _malloc(tf_bufsiz(t));
		i32 ret = escape_seq(out, in);
		if(ret < 0)
		{
			return NULL;
		}

		*out_len = ret;
		return out;
	}

	return in;
}

static void sr_esc_free(char *s)
{
	if(_sr_flags & SR_INP_USE_ESCSEQ)
	{
		_free(s);
	}
}

static char *_sr_err_empty = "Search is empty!";
static char *_sr_err_search = "Invalid escape sequence in search field!";
static char *_sr_err_replace = "Invalid escape sequence in replace field!";

static void sr_search(void)
{
	u32 slen;
	char *search = sr_tf_esc(&_sr_tf_search, &slen);
	if(!search)
	{
		msg_show(MSG_ERROR, _sr_err_search);
		return;
	}

	sr_esc_free(search);
}

static void sr_replace(void)
{
	u32 search_len;
	char *search = sr_tf_esc(&_sr_tf_search, &search_len);
	if(!search)
	{
		msg_show(MSG_ERROR, _sr_err_search);
		return;
	}

	if(!search_len)
	{
		sr_esc_free(search);
		msg_show(MSG_ERROR, _sr_err_empty);
		return;
	}

	u32 replace_len;
	char *replace = sr_tf_esc(&_sr_tf_replace, &replace_len);
	if(!replace)
	{
		sr_esc_free(search);
		msg_show(MSG_ERROR, _sr_err_replace);
		return;
	}

	u32 input_len;
	char *input = tb_export(_tb, &input_len);
	re_param params =
	{
		input,
		input_len,
		search,
		search_len,
		replace,
		replace_len,
		_sr_flags
	};

	vec result;
	re_replace_all(&params, &result);
	_free(input);
	tb_sel_all(_tb);
	vec_pushbyte(&result, '\0');
	tb_insert(_tb, vec_data(&result));
	vec_destroy(&result);

	sr_esc_free(search);
	sr_esc_free(replace);
}

static void sr_chk_toggle(void)
{
	switch(_sr_focus)
	{
	case SR_INP_MATCH_CASE:
		_sr_flags ^= SR_MATCH_CASE;
		break;

	case SR_INP_WHOLE_WORDS:
		_sr_flags ^= SR_WHOLE_WORDS;
		break;

	case SR_INP_USE_ESCSEQ:
		_sr_flags ^= SR_USE_ESCSEQ;
		break;
	}
}

static u32 sr_max(void)
{
	return (_sr_flags & SR_REPLACE) ? SR_COUNT : (SR_COUNT - 1);
}

static void sr_prev(void)
{
	_sr_focus = dec_wrap(_sr_focus, sr_max());
}

static void sr_next(void)
{
	_sr_focus = inc_wrap(_sr_focus, sr_max());
}

static u32 sr_is_chk(void)
{
	return _sr_focus <= SR_INP_CHK_END;
}

static void sr_key_other(u32 key, u32 chr)
{
	switch(_sr_focus)
	{
	case SR_INP_SEARCH:
		tf_key(&_sr_tf_search, key, chr);
		break;

	case SR_INP_REPLACE:
		tf_key(&_sr_tf_replace, key, chr);
		break;
	}
}

static void sr_key(u32 key, u32 chr)
{
	switch(key)
	{
	case KEY_UP:
	case MOD_SHIFT | KEY_TAB:
		sr_prev();
		break;

	case KEY_DOWN:
	case KEY_TAB:
		sr_next();
		break;

	case KEY_LEFT:
		if(sr_is_chk())
		{
			sr_prev();
		}
		else
		{
			sr_key_other(key, chr);
		}
		break;

	case KEY_RIGHT:
		if(sr_is_chk())
		{
			sr_next();
		}
		else
		{
			sr_key_other(key, chr);
		}
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

	case KEY_RETURN:
		sr_chk_toggle();
		if(_sr_focus == SR_INP_REPLACE)
		{
			sr_replace();
		}
		else if(_sr_focus == SR_INP_SEARCH)
		{
			sr_search();
		}
		break;

	case KEY_SPACE:
		sr_chk_toggle();
		/* fall through */

	default:
		sr_key_other(key, chr);
		break;
	}
}
