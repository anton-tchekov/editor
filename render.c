static selection vsel;
static cursor vcursor;
static u8 in_comment;

static void ed_render_linenr(u32 start_y, u32 end_y)
{
	u32 x, y, lnr_max, lnr_width;
	u32 lines = tb_num_lines(tb);
	u32 lnr = tb->page_y + start_y;
	lnr_max = lnr + _screen_height;
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);

	for(y = start_y; y < end_y; ++y)
	{
		u32 color = (lnr == tb->sel.c[1].y) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_GRAY, COLOR_TABLE_BG);

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
	if(y < vsel.c[0].y || y > vsel.c[1].y)
	{
		return 0;
	}

	if(y == vsel.c[0].y && x < vsel.c[0].x)
	{
		return 0;
	}

	if(y == vsel.c[1].y && x >= vsel.c[1].x)
	{
		return 0;
	}

	return 1;
}

static u32 is_cursor(u32 x, u32 y)
{
	return y == vcursor.y && x == vcursor.x;
}

static void ed_put(u32 x, u32 y, u32 c)
{
	if(is_cursor(x, y))
	{
		c = screen_pack_color_swap(c);
	}
	else if(is_sel(x, y))
	{
		c = screen_pack_set_bg(c, COLOR_TABLE_SELECTION);
	}

	screen_set(x + _offset_x, y - tb->page_y, c);
}

static u32 ed_syntax_sub(u32 c, u32 color, u32 y, u32 x)
{
	if(c == '\t')
	{
		u32 n = x & (_tabsize - 1);
		if(_show_whitespace)
		{
			color = screen_color(COLOR_TABLE_GRAY, COLOR_TABLE_BG);
			if(n == _tabsize - 1)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_BOTH, color));
			}
			else
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_START, color));
				for(++n; n < _tabsize - 1; ++n)
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
			color = COLOR_TABLE_GRAY;
		}

		if(x >= _page_w) { return x; }
		ed_put(x++, y, screen_pack(c, screen_color(color, COLOR_TABLE_BG)));
	}

	return x;
}

static u32 ed_plain(u32 y)
{
	vector *lv = tb_get_line(tb, y);
	char *line = vector_data(lv);
	u32 len = vector_len(lv);
	u32 i, x;
	for(x = 0, i = 0; i < len; ++i)
	{
		x = ed_syntax_sub(line[i], COLOR_TABLE_FG, y, x);
		if(x >= _page_w) { return x; }
	}

	return x;
}

static u32 ed_asm6800(u32 y)
{
	vector *lv = tb_get_line(tb, y);
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
				x = ed_syntax_sub(line[i], COLOR_TABLE_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if(c == '\"' || c == '\'')
		{
			u32 save = c;
			u32 esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_TABLE_STRING, y, x);
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
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 color, end, start;
			for(start = i; i < len && (is_asm_ident(c = line[i])); ++i) {}
			end = i;
			color = screen_color(
				keyword_detect(&asm_hashmap, line + start, end - start),
				COLOR_TABLE_BG);

			for(i = start; i < end; ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(line[i], color));
			}
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(line[i],
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '$')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			for(++i; i < len && isxdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else if(isdigit(c))
		{
			for(; i < len && isdigit((c = line[i])); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_TABLE_FG, y, x);
			if(x >= _page_w) { return x; }
			++i;
		}
	}

	return x;
}

static u32 ed_syntax(u32 y)
{
	vector *lv = tb_get_line(tb, y);
	u32 len = vector_len(lv);
	char *line = vector_data(lv);
	u32 incflag = 0;
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		u32 c = line[i];
		if(in_comment)
		{
			x = ed_syntax_sub(c, COLOR_TABLE_COMMENT, y, x);
			if(x >= _page_w) { return x; }

			if((c == '*') && (i + 1 < len) && (line[i + 1] == '/'))
			{
				++i;
				in_comment = 0;
				ed_put(x++, y, screen_pack('/',
					screen_color(COLOR_TABLE_COMMENT, COLOR_TABLE_BG)));
				if(x >= _page_w) { return x; }
			}
			++i;
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], COLOR_TABLE_COMMENT, y, x);
				if(x >= _page_w) { return x; }
			}
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '*'))
		{
			in_comment = 1;
		}
		else if(c == '#')
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= _page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save = c == '<' ? '>' : c;
			u32 esc = 0;
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_TABLE_STRING, y, x);
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
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_bracket(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_brace(c))
		{
			if(x >= _page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACE, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_ident_start(c))
		{
			u32 color, end, start;
			for(start = i; i < len && (is_ident(c = line[i])); ++i) {}
			end = i;
			color = screen_color(
				keyword_detect(&c_hashmap, line + start, end - start),
				COLOR_TABLE_BG);

			if(color == COLOR_TABLE_FG)
			{
				if(c == '(')
				{
					color = COLOR_TABLE_FN;
				}
				else if(c == '[')
				{
					color = COLOR_TABLE_ARRAY;
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
			u32 e = i + 1;
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
				ed_put(x, y, screen_pack(line[i],
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_TABLE_FG, y, x);
			if(x >= _page_w) { return x; }
			++i;
		}
	}

	return x;
}

static void ed_render_line(u32 y)
{
	u32 x = 0;
	u32 line = tb->page_y + y;
	if(line < tb_num_lines(tb))
	{
		switch(tb->language)
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
		ed_put(x++, line, screen_pack(' ',
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
	}

	for(; x < _page_w; ++x)
	{
		screen_set(x + _offset_x, y,
			screen_pack(' ', screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
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

static u32 dropdown_color(dropdown *d, u32 i)
{
	return (i == d->pos) ?
		screen_color(COLOR_TABLE_ERROR, COLOR_TABLE_GRAY) :
		screen_color(COLOR_TABLE_FG, COLOR_TABLE_GRAY);
}

static u32 ed_render_dir(u32 y)
{
	u32 i, end;
	end = umin(dropdown_nav.offset + DROPDOWN_PAGE, dropdown_nav.count);
	for(i = dropdown_nav.offset; i < end; ++i, ++y)
	{
		ed_render_line_str(dir_list[i], 0, y, dropdown_color(&dropdown_nav, i));
	}

	return y;
}

static void ed_render_nav(field *f, u32 y, char *prompt)
{
	char *s;
	u32 x, i, c, color;

	color = screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG);
	for(s = prompt, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, screen_pack(c, color));
	}

	for(s = f->buf, i = 0; x < _screen_width; ++x, ++s, ++i)
	{
		screen_set(x, y, screen_pack((i < f->len) ? *s : ' ',
			(i == f->cursor) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG)));
	}
}

static u32 ed_prev_comment(void)
{
	u32 i = 0, result = 0;
	if(tb->page_y > COMMENT_LOOKBACK)
	{
		i = tb->page_y - COMMENT_LOOKBACK;
	}

	for(; i < tb->page_y; ++i)
	{
		i32 p;
		vector *line = tb_get_line(tb, i);
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
	u32 lines = tb_num_lines(tb);
	if(lines > _screen_height && tb->page_y + _screen_height >= lines)
	{
		tb->page_y = lines - _screen_height;
	}

	vcursor.x = tb_cursor_pos_x(tb, tb->sel.c[1].y, tb->sel.c[1].x);
	vcursor.y = tb->sel.c[1].y;
	vsel.c[0] = vcursor;
	if(tb->sel.c[0].y == tb->sel.c[1].y && tb->sel.c[1].x == tb->sel.c[0].x)
	{
		vsel.c[1] = vcursor;
	}
	else
	{
		vsel.c[1].x = tb_cursor_pos_x(tb, tb->sel.c[0].y, tb->sel.c[0].x);
		vsel.c[1].y = tb->sel.c[0].y;
	}

	sel_norm(&vsel);
	if(tb->language == LANGUAGE_C)
	{
		in_comment = ed_prev_comment();
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
		" CTRL+B to view open buffers\0"
		" CTRL+G to go to line number or symbol definiton\0\1";

	u32 x, color = screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG);
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
