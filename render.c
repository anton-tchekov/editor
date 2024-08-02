static selection _vsel;
static cursor _vcursor;
static u8 _in_comment;

static void ed_render_linenr(u32 start_y, u32 end_y)
{
	u32 x, y, lnr_max, lnr_width, lines, lnr;

	lines = tb_num_lines(_tb);
	lnr = _tb->page_y + start_y + 1;
	lnr_max = lnr + (end_y - start_y - 1);
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);
	for(y = start_y; y < end_y && lnr <= lines; ++y, ++lnr)
	{
		u32 fg = COLOR_GRAY;
		u32 bg = COLOR_BG;
		if(lnr == _tb->sel.c[1].y + 1)
		{
			fg = COLOR_FG;
			bg = COLOR_BG;
		}

		char lnr_buf[16];
		linenr_str(lnr_buf, lnr, lnr_width);
		for(x = 0; x < lnr_width; ++x)
		{
			render_char(x, y, lnr_buf[x], fg, bg);
		}
	}

	_page_w = _screen_width - lnr_width - 1;
	_offset_x = lnr_width + 1;
}

static u32 is_sel(u32 x, u32 y)
{
	if(y < _vsel.c[0].y || y > _vsel.c[1].y)
	{
		return 0;
	}

	if(y == _vsel.c[0].y && x < _vsel.c[0].x)
	{
		return 0;
	}

	if(y == _vsel.c[1].y && x >= _vsel.c[1].x)
	{
		return 0;
	}

	return 1;
}

static void ed_put(u32 x, u32 y, u32 c, u32 fg, u32 bg)
{
	if(is_sel(x, y))
	{
		bg = COLOR_SEL;
	}

	render_char(x + _offset_x, y - _tb->page_y, c, fg, bg);
}

static u32 ed_syntax_sub(u32 c, u32 fg, u32 y, u32 x)
{
	if(c == '\t')
	{
		u32 n;

		n = x & (_tabsize - 1);
		if(_show_whitespace)
		{
			if(n == (u32)_tabsize - 1)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, CHAR_TAB_BOTH, COLOR_GRAY, COLOR_BG);
			}
			else
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, CHAR_TAB_START, COLOR_GRAY, COLOR_BG);
				for(++n; n < (u32)_tabsize - 1; ++n)
				{
					if(x >= _page_w) { return x; }
					ed_put(x++, y, CHAR_TAB_MIDDLE, COLOR_GRAY, COLOR_BG);
				}
				if(x >= _page_w) { return x; }
				ed_put(x++, y, CHAR_TAB_END, COLOR_GRAY, COLOR_BG);
			}
		}
		else
		{
			for(; n < _tabsize; ++n)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, ' ', fg, COLOR_BG);
			}
		}
	}
	else
	{
		if(c == ' ' && _show_whitespace)
		{
			c = CHAR_VISIBLE_SPACE;
			fg = COLOR_GRAY;
		}

		if(x >= _page_w) { return x; }
		ed_put(x++, y, c, fg, COLOR_BG);
	}

	return x;
}

static u32 ed_plain(u32 y)
{
	u32 i, x, len;
	vec *lv;
	char *line;

	lv = tb_get_line(_tb, y);
	line = vec_data(lv);
	len = vec_len(lv);
	for(x = 0, i = 0; i < len; ++i)
	{
		x = ed_syntax_sub(line[i], COLOR_FG, y, x);
		if(x >= _page_w) { return x; }
	}

	return x;
}

static u32 ed_asm(u32 y, hashmap *kw)
{
	u32 len, i, x, c;
	vec *lv;
	char *line;

	lv = tb_get_line(_tb, y);
	len = vec_len(lv);
	line = vec_data(lv);
	i = 0;
	x = 0;
	while(i < len)
	{
		c = line[i];
		if(c == ';')
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], COLOR_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if(c == '\"' || c == '\'')
		{
			u32 save, esc;

			save = c;
			esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_STRING, COLOR_BG);
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_STRING, y, x);
				if(x >= _page_w) { return x; }

				if(esc)
				{
					esc = 0;
				}
				else if(c == '\\')
				{
					esc = 1;
				}
				else if(c == save)
				{
					break;
				}
			}
			++i;
		}
		else if(is_paren(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_PAREN, COLOR_BG);
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 fg, end, start;

			for(start = i; i < len && (is_asm_ident(c = line[i])); ++i) {}
			end = i;
			fg = kw_detect(kw, line + start, end - start);
			for(i = start; i < end; ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, line[i], fg, COLOR_BG);
			}
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, line[i], COLOR_KEYWORD, COLOR_BG);
			++i;
		}
		else if(c == '$')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_NUMBER, COLOR_BG);
			for(++i; i < len && isxdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, c, COLOR_NUMBER, COLOR_BG);
			}
		}
		else if(isdigit(c))
		{
			for(; i < len && isdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, c, COLOR_NUMBER, COLOR_BG);
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_FG, y, x);
			if(x >= _page_w) { return x; }
			++i;
		}
	}

	return x;
}

static u32 ed_syntax(u32 y)
{
	u32 len, incflag, i, x, c;
	vec *lv;
	char *line;

	lv = tb_get_line(_tb, y);
	len = vec_len(lv);
	line = vec_data(lv);

	incflag = 0;
	i = 0;
	x = 0;
	while(i < len)
	{
		c = line[i];
		if(_in_comment)
		{
			x = ed_syntax_sub(c, COLOR_COMMENT, y, x);
			if(x >= _page_w) { return x; }

			if((c == '*') && (i + 1 < len) && (line[i + 1] == '/'))
			{
				++i;
				_in_comment = 0;
				ed_put(x++, y, '/', COLOR_COMMENT, COLOR_BG);
				if(x >= _page_w) { return x; }
			}
			++i;
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], COLOR_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '*'))
		{
			_in_comment = 1;
			ed_put(x++, y, '/', COLOR_COMMENT, COLOR_BG);
			if(x >= _page_w) { return x; }
			ed_put(x++, y, '*', COLOR_COMMENT, COLOR_BG);
			if(x >= _page_w) { return x; }
			i += 2;
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_KEYWORD, COLOR_BG);
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, c, COLOR_KEYWORD, COLOR_BG);
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save, esc;

			save = c == '<' ? '>' : c;
			esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_STRING, COLOR_BG);
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_STRING, y, x);
				if(x >= _page_w) { return x; }
				if(esc)
				{
					esc = 0;
				}
				else if(c == '\\')
				{
					esc = 1;
				}
				else if(c == save)
				{
					break;
				}
			}
			++i;
		}
		else if(is_paren(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_PAREN, COLOR_BG);
			++i;
		}
		else if(is_bracket(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_BRACKET, COLOR_BG);
			++i;
		}
		else if(is_brace(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, c, COLOR_BRACE, COLOR_BG);
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 fg, end, start;

			for(start = i; i < len && (is_ident(c = line[i])); ++i) {}
			end = i;
			fg = kw_detect(&_kw_c, line + start, end - start);
			if(fg == COLOR_FG)
			{
				if(c == '(')
				{
					fg = COLOR_FN;
				}
				else if(c == '[')
				{
					fg = COLOR_ARRAY;
				}
			}

			for(i = start; i < end; ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, line[i], fg, COLOR_BG);
			}
		}
		else if(isdigit(c))
		{
			u32 e;

			e = i + 1;
			if(c == '0')
			{
				if(e < len)
				{
					c = line[e];
					if(c == 'x' || c == 'X')
					{
						for(++e; e < len && isxdigit(line[e]); ++e) {}
					}
					else if(c == 'b' || c == 'B')
					{
						for(++e; e < len && is_bin(line[e]); ++e) {}
					}
					else
					{
						for(; e < len && is_oct(line[e]); ++e) {}
					}
				}
			}
			else
			{
				for(; e < len && isdigit(line[e]); ++e) {}
			}

			for(; i < e; ++x, ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x, y, line[i], COLOR_NUMBER, COLOR_BG);
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_FG, y, x);
			if(x >= _page_w) { return x; }
			++i;
		}
	}

	return x;
}

static void ed_render_line(u32 y)
{
	u32 x = 0;
	u32 line = _tb->page_y + y;
	if(line < tb_num_lines(_tb))
	{
		switch(_tb->language)
		{
		case LANGUAGE_UNKNOWN:
			x = ed_plain(line);
			break;

		case LANGUAGE_C:
			x = ed_syntax(line);
			break;

		case LANGUAGE_ASM6800:
			x = ed_asm(line, &_kw_asm_6800);
			break;

		case LANGUAGE_ASM65C02:
			x = ed_asm(line, &_kw_asm_65C02);
			break;
		}
	}

	if(x < _page_w)
	{
		ed_put(x++, line, ' ', COLOR_FG, COLOR_BG);
	}
}

static u32 ed_prev_comment(void)
{
	u32 i, result;

	i = 0;
	result = 0;
	if(_tb->page_y > COMMENT_LOOKBACK)
	{
		i = _tb->page_y - COMMENT_LOOKBACK;
	}

	for(; i < _tb->page_y; ++i)
	{
		i32 p, len;
		vec *line;
		char *data;

		line = tb_get_line(_tb, i);
		data = vec_data(line);
		len = vec_len(line);
		for(p = 0; p < len - 1; ++p)
		{
			if(result)
			{
				if(data[p] == '*' && data[p + 1] == '/')
				{
					++p;
					result = 0;
				}
			}
			else
			{
				if(data[p] == '/' && data[p + 1] == '*')
				{
					++p;
					result = 1;
				}
			}
		}
	}

	return result;
}

static void ed_render_buffer(u32 start_y, u32 end_y)
{
	u32 lines;

	lines = tb_num_lines(_tb);
	if(lines > _screen_height && _tb->page_y + _screen_height >= lines)
	{
		_tb->page_y = lines - _screen_height;
	}

	_vcursor.x = tb_cursor_pos_x(_tb, _tb->sel.c[1].y, _tb->sel.c[1].x);
	_vcursor.y = _tb->sel.c[1].y;
	_vsel.c[0] = _vcursor;
	if(_tb->sel.c[0].y == _tb->sel.c[1].y && _tb->sel.c[1].x == _tb->sel.c[0].x)
	{
		_vsel.c[1] = _vcursor;
	}
	else
	{
		_vsel.c[1].x = tb_cursor_pos_x(_tb, _tb->sel.c[0].y, _tb->sel.c[0].x);
		_vsel.c[1].y = _tb->sel.c[0].y;
	}

	sel_norm(&_vsel);
	if(_tb->language == LANGUAGE_C)
	{
		_in_comment = ed_prev_comment();
	}

	if(_show_linenr)
	{
		ed_render_linenr(start_y, end_y);
	}
	else
	{
		_page_w = _screen_width;
		_offset_x = 0;
	}

#if 0
	{
		u32 len = 0;
		char *s = tb_cur_line_span(_tb, &len);
		printf("%.*s - %d\n", len, s, len);
	}
#endif

	for(u32 y = start_y; y < end_y; ++y)
	{
		ed_render_line(y);
	}

	u32 pos_y = _vcursor.y - _tb->page_y;
	if(pos_y >= start_y && pos_y < end_y)
	{
		render_cursor(_vcursor.x + _offset_x, pos_y, COLOR_CURSOR);
	}
}

static void ed_render_blank(u32 start_y, u32 end_y)
{
	u32 x;
	char *help;

	help =
		"Editor \"Haven't come up with a good name yet\" V0.9\0"
		"  by Anton Tchekov\0"
		"\0"
		"CTRL+N or CTRL+T to create a new buffer\0"
		"CTRL+O to open file\0"
		"CTRL+S to save\0"
		"CTRL+SHIFT+S to save under a different name\0"
		"CTRL+W to discard buffer\0"
		"CTRL+B or CTRL+P to view open buffers\0"
		"CTRL+G to go to line number or symbol definiton\0"
		"CTRL+L to select the current line\0"
		"CTRL+SHIFT+L to toggle line number visibility\0"
		"CTRL+J to toggle whitespace visibility\0"
		"CTRL+SCROLL to change font size\0"
		"CTRL+SHIFT+SCROLL to change line height\0"
		"CTRL+D to trim trailing space\0"
		"CTRL+E to align #define's\0"
		"CTRL+I to insert #include \"..\"\0"
		"CTRL+SHIFT+I to insert #include <..>\0"
		"\1";

	for(; start_y < end_y; ++start_y)
	{
		if(start_y >= 15 && *help != 1)
		{
			for(x = 1; *help && x < _screen_width; ++help, ++x)
			{
				render_char(x, start_y, *help, COLOR_FG, COLOR_BG);
			}
			++help;
		}
	}
}

static void msg_render(void)
{
	char *out = _msg_buf;

	if(_msg_type == MSG_STATUS)
	{
		if(_tb)
		{
			int rlen = 0;
			char buf[128];
			if(sel_wide(&_tb->sel))
			{
				int x1 = tb_cursor_pos_x(_tb, _tb->sel.c[0].y, _tb->sel.c[0].x);
				int x2 = tb_cursor_pos_x(_tb, _tb->sel.c[1].y, _tb->sel.c[1].x);
				int y1 = _tb->sel.c[0].y + 1;
				int y2 = _tb->sel.c[1].y + 1;
				int dx = abs(x1 - x2);
				int dy = abs(y1 - y2) + 1;
				rlen = snprintf(buf, sizeof(buf),
					"   [%4d|%4d] [%4d|%4d] [%4d|%4d] [%s]",
					dy, dx, y1, x1, y2, x2, lang_str(_tb->language));
			}
			else
			{
				rlen = snprintf(buf, sizeof(buf), "   [%4d|%4d] [%s]",
					_tb->sel.c[0].y + 1,
					tb_cursor_pos_x(_tb, _tb->sel.c[0].y, _tb->sel.c[0].x),
					lang_str(_tb->language));
			}

			int len = snprintf(_msg_buf, sizeof(_msg_buf), "%s%s [%d Lines]",
				_tb->filename, _tb->modified ? "*" : "", tb_num_lines(_tb));

			memset(_msg_buf + len, ' ', _screen_width - len);
			memcpy(_msg_buf + _screen_width - rlen, buf, rlen);
			_msg_buf[_screen_width] = '\0';
		}
		else
		{
			out = "Version 0.9";
		}
	}

	ed_render_line_str(out, 0, _full_height - 1, COLOR_FG,
		msg_color(_msg_type));

	msg_tryclose();
}
