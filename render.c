static selection _vsel;
static cursor _vcursor;
static u8 _in_comment;

static void ed_render_linenr(u32 start_y, u32 end_y)
{
	u32 x, y, lnr_max, lnr_width;
	u32 lines = tb_num_lines(_tb);
	u32 lnr = _tb->page_y + start_y;
	lnr_max = lnr + _screen_height;
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);

	for(y = start_y; y < end_y; ++y)
	{
		u32 color = (lnr == _tb->sel.c[1].y) ?
			ptp(PT_FG, PT_BG) : ptp(PT_GRAY, PT_BG);

		++lnr;
		if(lnr <= lines)
		{
			char lnr_buf[16];
			linenr_str(lnr_buf, lnr, lnr_width);
			for(x = 0; x < lnr_width; ++x)
			{
				screen_set(x, y, screen_pack(lnr_buf[x], color));
			}
		}
		else
		{
			for(x = 0; x < lnr_width; ++x)
			{
				screen_set(x, y, screen_pack(' ', color));
			}
		}

		screen_set(x, y, screen_pack(' ', color));
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

static u32 is_cursor(u32 x, u32 y)
{
	return y == _vcursor.y && x == _vcursor.x;
}

static void ed_put(u32 x, u32 y, u32 c)
{
	if(is_cursor(x, y))
	{
		c = screen_pack_color_swap(c);
	}
	else if(is_sel(x, y))
	{
		c = screen_pack_set_bg(c, PT_SEL);
	}

	screen_set(x + _offset_x, y - _tb->page_y, c);
}

static u32 ed_syntax_sub(u32 c, u32 color, u32 y, u32 x)
{
	if(c == '\t')
	{
		u32 n = x & (_tabsize - 1);
		if(_show_whitespace)
		{
			color = ptp(PT_GRAY, PT_BG);
			if(n == (u32)_tabsize - 1)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_BOTH, color));
			}
			else
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_START, color));
				for(++n; n < (u32)_tabsize - 1; ++n)
				{
					if(x >= _page_w) { return x; }
					ed_put(x++, y, screen_pack(CHAR_TAB_MIDDLE, color));
				}
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_END, color));
			}
		}
		else
		{
			for(; n < _tabsize; ++n)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(' ', color));
			}
		}
	}
	else
	{
		if(c == ' ' && _show_whitespace)
		{
			c = CHAR_VISIBLE_SPACE;
			color = PT_GRAY;
		}

		if(x >= _page_w) { return x; }
		ed_put(x++, y, screen_pack(c, ptp(color, PT_BG)));
	}

	return x;
}

static u32 ed_plain(u32 y)
{
	vector *lv = tb_get_line(_tb, y);
	char *line = vector_data(lv);
	u32 len = vector_len(lv);
	u32 i, x;
	for(x = 0, i = 0; i < len; ++i)
	{
		x = ed_syntax_sub(line[i], PT_FG, y, x);
		if(x >= _page_w) { return x; }
	}

	return x;
}

static u32 ed_asm6800(u32 y)
{
	vector *lv = tb_get_line(_tb, y);
	u32 len = vector_len(lv);
	char *line = vector_data(lv);
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		u32 c = line[i];
		if(c == ';')
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], PT_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if(c == '\"' || c == '\'')
		{
			u32 save = c;
			u32 esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_STRING, PT_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, PT_STRING, y, x);
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
			ed_put(x++, y, screen_pack(c, ptp(PT_PAREN, PT_BG)));
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 color, end, start;

			for(start = i; i < len && (is_asm_ident(c = line[i])); ++i) {}
			end = i;
			color = ptp(kw_detect(&_kw_asm, line + start, end - start), PT_BG);
			for(i = start; i < end; ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(line[i], color));
			}
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(line[i], ptp(PT_KEYWORD, PT_BG)));
			++i;
		}
		else if(c == '$')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_NUMBER, PT_BG)));
			for(++i; i < len && isxdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c, ptp(PT_NUMBER, PT_BG)));
			}
		}
		else if(isdigit(c))
		{
			for(; i < len && isdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c, ptp(PT_NUMBER, PT_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, PT_FG, y, x);
			if(x >= _page_w) { return x; }
			++i;
		}
	}

	return x;
}

static u32 ed_syntax(u32 y)
{
	vector *lv = tb_get_line(_tb, y);
	u32 len = vector_len(lv);
	char *line = vector_data(lv);
	u32 incflag = 0;
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		u32 c = line[i];
		if(_in_comment)
		{
			x = ed_syntax_sub(c, PT_COMMENT, y, x);
			if(x >= _page_w) { return x; }

			if((c == '*') && (i + 1 < len) && (line[i + 1] == '/'))
			{
				++i;
				_in_comment = 0;
				ed_put(x++, y, screen_pack('/', ptp(PT_COMMENT, PT_BG)));
				if(x >= _page_w) { return x; }
			}
			++i;
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], PT_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '*'))
		{
			_in_comment = 1;
			ed_put(x++, y, screen_pack('/', ptp(PT_COMMENT, PT_BG)));
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack('*', ptp(PT_COMMENT, PT_BG)));
			if(x >= _page_w) { return x; }
			i += 2;
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_KEYWORD, PT_BG)));
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c, ptp(PT_KEYWORD, PT_BG)));
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save = c == '<' ? '>' : c;
			u32 esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_STRING, PT_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, PT_STRING, y, x);
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
			ed_put(x++, y, screen_pack(c, ptp(PT_PAREN, PT_BG)));
			++i;
		}
		else if(is_bracket(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_BRACKET, PT_BG)));
			++i;
		}
		else if(is_brace(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c, ptp(PT_BRACE, PT_BG)));
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 color, end, start;

			for(start = i; i < len && (is_ident(c = line[i])); ++i) {}
			end = i;
			color = ptp(kw_detect(&_kw_c, line + start, end - start), PT_BG);
			if(color == PT_FG)
			{
				if(c == '(')
				{
					color = PT_FN;
				}
				else if(c == '[')
				{
					color = PT_ARRAY;
				}
			}

			for(i = start; i < end; ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(line[i], color));
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
				ed_put(x, y, screen_pack(line[i], ptp(PT_NUMBER, PT_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, PT_FG, y, x);
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
			x = ed_asm6800(line);
			break;
		}
	}

	if(x < _page_w)
	{
		ed_put(x++, line, screen_pack(' ', ptp(PT_FG, PT_BG)));
	}

	for(; x < _page_w; ++x)
	{
		screen_set(x + _offset_x, y, screen_pack(' ', ptp(PT_FG, PT_BG)));
	}
}

static void ed_render_line_str(char *s, u32 x, u32 y, u32 color)
{
	for(; *s && x < _screen_width; ++s, ++x)
	{
		screen_set(x, y, screen_pack(*s, color));
	}

	for(; x < _screen_width; ++x)
	{
		screen_set(x, y, screen_pack(' ', color));
	}
}

static u32 ed_prev_comment(void)
{
	u32 i = 0, result = 0;
	if(_tb->page_y > COMMENT_LOOKBACK)
	{
		i = _tb->page_y - COMMENT_LOOKBACK;
	}

	for(; i < _tb->page_y; ++i)
	{
		i32 p;
		vector *line = tb_get_line(_tb, i);
		char *data = vector_data(line);
		i32 len = vector_len(line);
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
	u32 lines = tb_num_lines(_tb);
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

	for(; start_y < end_y; ++start_y)
	{
		ed_render_line(start_y);
	}
}

static void ed_render_blank(u32 start_y, u32 end_y)
{
	char *help =
		" Editor \"Haven't come up with a good name yet\" V0.9\0"
		"   by Anton Tchekov\0"
		"\0"
		" CTRL+N or CTRL+T to create a new buffer\0"
		" CTRL+O to open file\0"
		" CTRL+S to save\0"
		" CTRL+W to discard buffer\0"
		" CTRL+B or CTRL+P to view open buffers\0"
		" CTRL+G to go to line number or symbol definiton\0\1";

	u32 x, color = ptp(PT_FG, PT_BG);
	for(; start_y < end_y; ++start_y)
	{
		if(start_y < 20 || *help == 1)
		{
			for(x = 0; x < _screen_width; ++x)
			{
				screen_set(x, start_y, screen_pack(' ', color));
			}
		}
		else
		{
			ed_render_line_str(help, 0, start_y, color);
			help += strlen(help) + 1;
		}
	}
}
