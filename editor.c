#include "util.c"
#include "keyword.c"
#include "test.c"

enum
{
	ED_MODE_DEFAULT,
	ED_MODE_NAV,
	ED_MODE_MSG,
	ED_MODE_OPENED,
	ED_MODE_COUNT
};

enum
{
	ED_INFO,
	ED_ERROR
};

enum
{
	LANGUAGE_UNKNOWN,
	LANGUAGE_C,
	LANGUAGE_DEFAULT = LANGUAGE_C,
	LANGUAGE_ASM6800,
	LANGUAGE_COUNT
};

#ifndef NDEBUG
static u32 render_cnt;
#endif

#define MAX_SEARCH_LEN    256
#define MAX_MSG_LEN        80
#define ED_DIR_PAGE        12
#define COMMENT_LOOKBACK   10

#include "cursor.c"

static u32 offset_x, page_w;
static u32 tabsize;

#include "textbuf.c"

static selection vsel;
static cursor vcursor;
static vector buffers;

static u32 cur_buf;
static u32 untitled_cnt = 1;

static u8 in_comment;
static u8 show_linenr;
static u8 show_whitespace;
static u8 mode;

static u8 msg_type;
static char msg_buf[MAX_MSG_LEN];

static u32 nav_cursor, nav_len;
static char *nav_buf, nav_base[MAX_SEARCH_LEN];

static u8 first_compare;
static char *search_file, same[MAX_SEARCH_LEN];

static char **dir_list;
static u32 dir_entries, dir_offset, dir_pos;

static textbuf *tb;

static textbuf *ed_tb_get(u32 i)
{
	return *(textbuf **)vector_get(&buffers, i * sizeof(textbuf *));
}

static u32 ed_tb_count(void)
{
	return vector_len(&buffers) / sizeof(textbuf *);
}

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

	page_w = _screen_width - lnr_width - 1;
	offset_x = lnr_width + 1;
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

	screen_set(x + offset_x, y - tb->page_y, c);
}

static u32 ed_syntax_sub(u32 c, u32 color, u32 y, u32 x)
{
	if(c == '\t')
	{
		u32 n = x & (tabsize - 1);
		if(show_whitespace)
		{
			color = screen_color(COLOR_TABLE_GRAY, COLOR_TABLE_BG);
			if(n == tabsize - 1)
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_BOTH, color));
			}
			else
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_START, color));
				for(++n; n < tabsize - 1; ++n)
				{
					if(x >= page_w) { return x; }
					ed_put(x++, y, screen_pack(CHAR_TAB_MIDDLE, color));
				}
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(CHAR_TAB_END, color));
			}
		}
		else
		{
			for(; n < tabsize; ++n)
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(' ', color));
			}
		}
	}
	else
	{
		if(c == ' ' && show_whitespace)
		{
			c = CHAR_VISIBLE_SPACE;
			color = COLOR_TABLE_GRAY;
		}

		if(x >= page_w) { return x; }
		ed_put(x++, y, screen_pack(c, screen_color(color, COLOR_TABLE_BG)));
	}

	return x;
}

static u32 ed_plain(u32 y)
{
	vector *lv = tb_get_line(tb, y);
	const char *line = vector_data(lv);
	u32 len = vector_len(lv);
	u32 i, x;
	for(x = 0, i = 0; i < len; ++i)
	{
		x = ed_syntax_sub(line[i], COLOR_TABLE_FG, y, x);
		if(x >= page_w) { return x; }
	}

	return x;
}

static u32 ed_asm6800(u32 y)
{
	vector *lv = tb_get_line(tb, y);
	u32 len = vector_len(lv);
	const char *line = vector_data(lv);
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
				if(x >= page_w) { return x; }
			}
		}
		else if(c == '\"' || c == '\'')
		{
			u32 save = c;
			u32 esc = 0;
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_TABLE_STRING, y, x);
				if(x >= page_w) { return x; }

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
			if(x >= page_w) { return x; }
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
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(line[i], color));
			}
		}
		else if(c == '#')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(line[i],
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '$')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			for(++i; i < len && isxdigit((c = line[i])); ++i)
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else if(isdigit(c))
		{
			for(; i < len && isdigit((c = line[i])); ++i)
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_TABLE_FG, y, x);
			if(x >= page_w) { return x; }
			++i;
		}
	}

	return x;
}

static u32 ed_syntax(u32 y)
{
	vector *lv = tb_get_line(tb, y);
	u32 len = vector_len(lv);
	const char *line = vector_data(lv);
	u32 incflag = 0;
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		u32 c = line[i];
		if(in_comment)
		{
			x = ed_syntax_sub(c, COLOR_TABLE_COMMENT, y, x);
			if(x >= page_w) { return x; }

			if((c == '*') && (i + 1 < len) && (line[i + 1] == '/'))
			{
				++i;
				in_comment = 0;
				ed_put(x++, y, screen_pack('/',
					screen_color(COLOR_TABLE_COMMENT, COLOR_TABLE_BG)));
				if(x >= page_w) { return x; }
			}
			++i;
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(line[i], COLOR_TABLE_COMMENT, y, x);
				if(x >= page_w) { return x; }
			}
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '*'))
		{
			in_comment = 1;
		}
		else if(c == '#')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= page_w) { return x; }
				ed_put(x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save = c == '<' ? '>' : c;
			u32 esc = 0;
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(c, COLOR_TABLE_STRING, y, x);
				if(x >= page_w) { return x; }

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
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_bracket(c))
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG)));
			++i;
		}
		else if(is_brace(c))
		{
			if(x >= page_w) { return x; }
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
				if(x >= page_w) { return x; }
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
				if(x >= page_w) { return x; }
				ed_put(x, y, screen_pack(line[i],
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(c, COLOR_TABLE_FG, y, x);
			if(x >= page_w) { return x; }
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

	if(x < page_w)
	{
		ed_put(x++, line, screen_pack(' ',
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
	}

	for(; x < page_w; ++x)
	{
		screen_set(x + offset_x, y,
			screen_pack(' ', screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
	}
}

static void ed_render_msg(void)
{
	const char *s = msg_buf;
	u32 x, c;
	u32 y = _screen_height - 1;
	u32 color = screen_color(COLOR_TABLE_FG,
		msg_type ? COLOR_TABLE_ERROR : COLOR_TABLE_INFO);

	for(x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, screen_pack(c, color));
	}

	for(; x < _screen_width; ++x)
	{
		screen_set(x, y, screen_pack(' ', color));
	}
}

static u32 ed_render_dir(void)
{
	u32 i;
	u32 y = 1;
	u32 end = dir_offset + ED_DIR_PAGE;
	if(end > dir_entries)
	{
		end = dir_entries;
	}

	for(i = dir_offset; i < end; ++i, ++y)
	{
		u32 color = (i == dir_pos) ?
			screen_color(COLOR_TABLE_ERROR, COLOR_TABLE_GRAY) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_GRAY);

		u32 x = 0;
		const char *p = dir_list[i];
		for(; *p && x < _screen_width; ++p, ++x)
		{
			screen_set(x, y, screen_pack(*p, color));
		}

		for(; x < _screen_width; ++x)
		{
			screen_set(x, y, screen_pack(' ', color));
		}
	}

	return y;
}

static u32 ed_render_opened(void)
{
	u32 i;
	u32 y = 0;
	u32 end = dir_offset + ED_DIR_PAGE;
	if(end > dir_entries)
	{
		end = dir_entries;
	}

	for(i = dir_offset; i < end; ++i, ++y)
	{
		textbuf *t = ed_tb_get(i);
		u32 color = (i == dir_pos) ?
			screen_color(COLOR_TABLE_ERROR, COLOR_TABLE_GRAY) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_GRAY);

		u32 x = 0;
		const char *p = t->filename;
		screen_set(x++, y, screen_pack(t->modified ? '*' : ' ', color));
		for(; *p && x < _screen_width; ++p, ++x)
		{
			screen_set(x, y, screen_pack(*p, color));
		}

		for(; x < _screen_width; ++x)
		{
			screen_set(x, y, screen_pack(' ', color));
		}
	}

	return y;
}

static u32 ed_render_nav(void)
{
	const char *prompt = "Location: ";
	const char *s;
	u32 x, i, c, color;

	color = screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG);
	for(s = prompt, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, 0, screen_pack(c, color));
	}

	for(s = nav_buf, i = 0; x < _screen_width; ++x, ++s, ++i)
	{
		screen_set(x, 0, screen_pack((i < nav_len) ? *s : ' ',
			(i == nav_cursor) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG)));
	}

	return ed_render_dir();
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
		const char *data = vector_data(line);
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

static void ed_render(void)
{
	u32 y;
	u32 start_y = 0;
	u32 end_y = _screen_height;
	u32 lines = tb_num_lines(tb);

	if(tb->sel.c[1].y < tb->page_y)
	{
		tb->page_y = tb->sel.c[1].y;
	}

	if(tb->sel.c[1].y >= tb->page_y + _screen_height)
	{
		tb->page_y = tb->sel.c[1].y - _screen_height + 1;
	}

	if(lines > _screen_height && tb->page_y + _screen_height >= lines)
	{
		tb->page_y = lines - _screen_height;
	}

	switch(mode)
	{
	case ED_MODE_NAV:
		start_y = ed_render_nav();
		break;

	case ED_MODE_MSG:
		--end_y;
		ed_render_msg();
		break;

	case ED_MODE_OPENED:
		start_y = ed_render_opened();
		break;
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

	if(show_linenr)
	{
		ed_render_linenr(start_y, end_y);
	}
	else
	{
		page_w = _screen_width;
		offset_x = 0;
	}

	for(y = start_y; y < end_y; ++y)
	{
		ed_render_line(y);
	}

#ifndef NDEBUG
	++render_cnt;
#endif
}

static void ed_msg(u32 type, const char *msg, ...)
{
	va_list args;
	mode = ED_MODE_MSG;
	msg_type = type;
	va_start(args, msg);
	vsnprintf(msg_buf, MAX_MSG_LEN, msg, args);
	va_end(args);
	ed_render();
}

static void ed_backspace(void)
{
	tb_backspace(tb);
	ed_render();
}

static void ed_delete(void)
{
	tb_delete(tb);
	ed_render();
}

static void ed_char(u32 chr)
{
	tb_char(tb, chr);
	ed_render();
}

static void ed_enter(void)
{
	tb_enter(tb);
	ed_render();
}

static void ed_enter_after(void)
{
	tb_enter_after(tb);
	ed_render();
}

static void ed_enter_before(void)
{
	tb_enter_before(tb);
	ed_render();
}

static void ed_sel_home(void)
{
	tb_sel_home(tb);
	ed_render();
}

static void ed_home(void)
{
	tb_home(tb);
	ed_render();
}

static void ed_sel_end(void)
{
	tb_sel_end(tb);
	ed_render();
}

static void ed_end(void)
{
	tb_end(tb);
	ed_render();
}

static void ed_sel_up(void)
{
	tb_sel_up(tb);
	ed_render();
}

static void ed_up(void)
{
	tb_up(tb);
	ed_render();
}

static void ed_sel_down(void)
{
	tb_sel_down(tb);
	ed_render();
}

static void ed_down(void)
{
	tb_down(tb);
	ed_render();
}

static void ed_sel_page_up(void)
{
	tb_sel_page_up(tb);
	ed_render();
}

static void ed_page_up(void)
{
	tb_page_up(tb);
	ed_render();
}

static void ed_sel_page_down(void)
{
	tb_sel_page_down(tb);
	ed_render();
}

static void ed_page_down(void)
{
	tb_page_down(tb);
	ed_render();
}

static void ed_sel_left(void)
{
	tb_sel_left(tb);
	ed_render();
}

static void ed_left(void)
{
	tb_left(tb);
	ed_render();
}

static void ed_sel_right(void)
{
	tb_sel_right(tb);
	ed_render();
}

static void ed_right(void)
{
	tb_right(tb);
	ed_render();
}

static void ed_sel_top(void)
{
	tb_sel_top(tb);
	ed_render();
}

static void ed_top(void)
{
	tb_top(tb);
	ed_render();
}

static void ed_sel_bottom(void)
{
	tb_sel_bottom(tb);
	ed_render();
}

static void ed_bottom(void)
{
	tb_bottom(tb);
	ed_render();
}

static u32 ed_switch_to(const char *fname)
{
	u32 i, len = ed_tb_count();
	for(i = 0; i < len; ++i)
	{
		textbuf *cur = ed_tb_get(i);
		if(!strcmp(cur->filename, fname))
		{
			cur_buf = i;
			tb = cur;
			return 1;
		}
	}

	return 0;
}

static void ed_load(const char *filename)
{
	char *buf;
	if(ed_switch_to(filename))
	{
		return;
	}

	switch(textfile_read(filename, &buf))
	{
	case FILE_READ_FAIL:
		ed_msg(ED_ERROR, "Failed to open file");
		return;

	case FILE_READ_NOT_TEXT:
		ed_msg(ED_ERROR, "Invalid character, binary file?");
		return;
	}

	tb = tb_new(filename, buf, 1, LANGUAGE_DEFAULT);
	vector_push(&buffers, sizeof(textbuf *), &tb);
	_free(buf);
	mode = ED_MODE_DEFAULT;
}

static void tbs_new(void)
{
	char name[32];
	snprintf(name, sizeof(name), "untitled-%d", untitled_cnt++);
	tb = tb_new(name, NULL, 0, LANGUAGE_DEFAULT);
	vector_push(&buffers, sizeof(textbuf *), &tb);
	mode = ED_MODE_DEFAULT;
}

static void ed_new_file(void)
{
	tbs_new();
	ed_render();
}

static void ed_init(void)
{
	vector_init(&buffers, 8 * sizeof(textbuf *));
	tbs_new();
	tabsize = 4;
	show_whitespace = 1;
	show_linenr = 1;
	get_working_dir(nav_base);
	nav_buf = nav_base + strlen(nav_base);
}

static void ed_copy(void)
{
	u32 len;
	char *text = tb_sel_get(tb, &len);
	clipboard_store(text);
	_free(text);
}

static void ed_sel_all(void)
{
	tb_sel_all(tb);
	ed_render();
}

static void ed_cut(void)
{
	ed_copy();
	tb_sel_delete(tb);
	ed_render();
}

static void ed_paste(void)
{
	char *text = clipboard_load();
	tb_insert(tb, text);
	free(text);
	ed_render();
}

static void ed_sel_prev_word(void)
{
	tb_sel_prev_word(tb);
	ed_render();
}

static void ed_prev_word(void)
{
	tb_prev_word(tb);
	ed_render();
}

static void ed_del_prev_word(void)
{
	tb_del_prev_word(tb);
	ed_render();
}

static void ed_sel_next_word(void)
{
	tb_sel_next_word(tb);
	ed_render();
}

static void ed_next_word(void)
{
	tb_next_word(tb);
	ed_render();
}

static void ed_del_next_word(void)
{
	tb_del_next_word(tb);
	ed_render();
}

static void ed_del_cur_line(void)
{
	tb_del_cur_line(tb);
	ed_render();
}

static void ed_move_up(void)
{
	tb_move_up(tb);
	ed_render();
}

static void ed_move_down(void)
{
	tb_move_down(tb);
	ed_render();
}

static void ed_toggle_line_nr(void)
{
	show_linenr = !show_linenr;
	ed_render();
}

static void ed_toggle_lang(void)
{
	tb_toggle_lang(tb);
	ed_render();
}

static void ed_tab_size(void)
{
	tabsize <<= 1;
	if(tabsize > 8)
	{
		tabsize = 2;
	}
	ed_render();
}

static void ed_whitespace(void)
{
	show_whitespace = !show_whitespace;
	ed_render();
}

static void ed_save_as(void)
{
}

static void ed_save(void)
{
	if(!tb->modified)
	{
		return;
	}

	if(tb->exists)
	{
		u32 len;
		char *buf = tb_export(tb, &len);
		if(file_write(tb->filename, buf, len))
		{
			ed_msg(ED_ERROR, "Writing file failed");
		}
		else
		{
			ed_msg(ED_INFO, "File saved");
			tb->modified = 0;
		}
		_free(buf);
	}
	else
	{
		ed_save_as();
	}
}

static void ed_ins_comment(void)
{
	tb_ins_comment(tb);
	ed_render();
}

static void ed_ins_include(void)
{
	tb_ins_include(tb);
	ed_render();
}

static void ed_ins_include_lib(void)
{
	tb_ins_include_lib(tb);
	ed_render();
}

static void ed_sel_cur_line(void)
{
	tb_sel_cur_line(tb);
	ed_render();
}

static void ed_trailing(void)
{
	tb_remove_trailing_space(tb);
	ed_render();
}

static void ed_close_file(void)
{
}

static u32 ed_has_unsaved(void)
{
	u32 i, len = ed_tb_count();
	for(i = 0; i < len; ++i)
	{
		if(ed_tb_get(i)->modified)
		{
			return 1;
		}
	}

	return 0;
}

static void ed_cleanup(void)
{
	u32 i, len = ed_tb_count();
	for(i = 0; i < len; ++i)
	{
		tb_destroy(ed_tb_get(i));
	}

	vector_destroy(&buffers);
	_free(dir_list);
}

static void ed_mode_opened(void)
{
	mode = ED_MODE_OPENED;
	dir_pos = 0;
	dir_offset = 0;
	dir_entries = ed_tb_count();
}

static u32 ed_quit_internal(void)
{
	if(ed_has_unsaved())
	{
		ed_mode_opened();
		ed_render();
		return 0;
	}

	ed_cleanup();
	return 1;
}

static void ed_quit(void)
{
	if(ed_quit_internal())
	{
		request_exit();
	}
}

static void ed_switch_buf(void)
{
	u32 cnt = ed_tb_count();
	if(cnt == 1)
	{
		return;
	}

	++cur_buf;
	if(cur_buf == cnt)
	{
		cur_buf = 0;
	}

	tb = ed_tb_get(cur_buf);
	ed_render();
}

static void ed_opened(void)
{
	ed_mode_opened();
	ed_render();
}

static void event_scroll(i32 y)
{
	tb->page_y += 3 * y;
	ed_render();
}

static void nav_up_fix_offset(void)
{
	if(dir_pos < dir_offset)
	{
		dir_offset = dir_pos;
	}
}

static void nav_down_fix_offset(void)
{
	if(dir_pos >= dir_offset + ED_DIR_PAGE)
	{
		dir_offset = (dir_pos < ED_DIR_PAGE) ? 0 : (dir_pos - ED_DIR_PAGE + 1);
	}
}

static void nav_up(void)
{
	if(dir_pos > 0)
	{
		--dir_pos;
		nav_up_fix_offset();
		ed_render();
	}
}

static void nav_down(void)
{
	if(dir_pos < dir_entries - 1)
	{
		++dir_pos;
		nav_down_fix_offset();
		ed_render();
	}
}

static void nav_page_up(void)
{
	if(dir_pos == 0)
	{
		return;
	}

	if(dir_pos > ED_DIR_PAGE)
	{
		dir_pos -= ED_DIR_PAGE;
		nav_up_fix_offset();
	}
	else
	{
		dir_pos = 0;
		dir_offset = 0;
	}

	ed_render();
}

static void nav_page_down(void)
{
	if(dir_pos < dir_entries - 1)
	{
		dir_pos += ED_DIR_PAGE;
		if(dir_pos > dir_entries - 1)
		{
			dir_pos = dir_entries - 1;
		}
		nav_down_fix_offset();
		ed_render();
	}
}

static void nav_first(void)
{
	if(dir_pos > 0)
	{
		dir_pos = 0;
		dir_offset = 0;
		ed_render();
	}
}

static void nav_last(void)
{
	if(dir_pos < dir_entries - 1)
	{
		dir_pos = dir_entries - 1;
		nav_down_fix_offset();
		ed_render();
	}
}

#include "nav.c"

static void ed_key_press_default(u32 key, u32 cp)
{
#ifndef NDEBUG
	render_cnt = 0;
#endif

	if(!tb)
	{
		switch(key)
		{
		case MOD_CTRL | KEY_G:  ed_goto();      break;
		case MOD_CTRL | KEY_O:  ed_open();      break;
		case MOD_CTRL | KEY_T:
		case MOD_CTRL | KEY_N:  ed_new_file();  break;
		case MOD_CTRL | KEY_Q:  ed_quit();      break;
		case MOD_CTRL | KEY_B:  ed_opened();    break;
		}

		return;
	}

	switch(key)
	{
	case MOD_CTRL | MOD_SHIFT | KEY_LEFT:   ed_sel_prev_word();  break;
	case MOD_CTRL | KEY_LEFT:               ed_prev_word();      break;
	case MOD_SHIFT | KEY_LEFT:              ed_sel_left();       break;
	case KEY_LEFT:                          ed_left();           break;
	case MOD_CTRL | MOD_SHIFT | KEY_RIGHT:  ed_sel_next_word();  break;
	case MOD_CTRL | KEY_RIGHT:              ed_next_word();      break;
	case MOD_SHIFT | KEY_RIGHT:             ed_sel_right();      break;
	case KEY_RIGHT:                         ed_right();          break;
	case MOD_CTRL | KEY_UP:                 ed_move_up();        break;
	case MOD_SHIFT | KEY_UP:                ed_sel_up();         break;
	case KEY_UP:                            ed_up();             break;
	case MOD_CTRL | KEY_DOWN:               ed_move_down();      break;
	case MOD_SHIFT | KEY_DOWN:              ed_sel_down();       break;
	case KEY_DOWN:                          ed_down();           break;
	case MOD_SHIFT | KEY_PAGE_UP:           ed_sel_page_up();    break;
	case KEY_PAGE_UP:                       ed_page_up();        break;
	case MOD_SHIFT | KEY_PAGE_DOWN:         ed_sel_page_down();  break;
	case KEY_PAGE_DOWN:                     ed_page_down();      break;
	case MOD_CTRL | MOD_SHIFT | KEY_HOME:   ed_sel_top();        break;
	case MOD_CTRL | KEY_HOME:               ed_top();            break;
	case MOD_SHIFT | KEY_HOME:              ed_sel_home();       break;
	case KEY_HOME:                          ed_home();           break;
	case MOD_CTRL | MOD_SHIFT | KEY_END:    ed_sel_bottom();     break;
	case MOD_CTRL | KEY_END:                ed_bottom();         break;
	case MOD_SHIFT | KEY_END:               ed_sel_end();        break;
	case KEY_END:                           ed_end();            break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                        ed_enter();          break;
	case MOD_CTRL | KEY_RETURN:             ed_enter_after();    break;
	case MOD_CTRL | MOD_SHIFT | KEY_RETURN: ed_enter_before();   break;
	case MOD_CTRL | KEY_BACKSPACE:          ed_del_prev_word();  break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:                     ed_backspace();      break;
	case MOD_CTRL | KEY_DELETE:             ed_del_next_word();  break;
	case MOD_SHIFT | KEY_DELETE:            ed_del_cur_line();   break;
	case KEY_DELETE:                        ed_delete();         break;
	case MOD_CTRL | KEY_L:                  ed_sel_cur_line();   break;
	case MOD_CTRL | MOD_SHIFT | KEY_L:      ed_toggle_line_nr(); break;
	case MOD_CTRL | KEY_K:                  ed_toggle_lang();    break;
	case MOD_CTRL | KEY_G:                  ed_goto();           break;
	case MOD_CTRL | KEY_O:                  ed_open();           break;
	case MOD_CTRL | KEY_C:                  ed_copy();           break;
	case MOD_CTRL | KEY_X:                  ed_cut();            break;
	case MOD_CTRL | KEY_V:                  ed_paste();          break;
	case MOD_CTRL | KEY_S:                  ed_save();           break;
	case MOD_CTRL | MOD_SHIFT | KEY_S:      ed_save_as();        break;
	case MOD_CTRL | MOD_SHIFT | KEY_T:      ed_tab_size();       break;
	case MOD_CTRL | KEY_T:
	case MOD_CTRL | KEY_N:                  ed_new_file();       break;
	case MOD_CTRL | KEY_W:                  ed_close_file();     break;
	case MOD_CTRL | KEY_Q:                  ed_quit();           break;
	case MOD_CTRL | KEY_J:                  ed_whitespace();     break;
	case MOD_CTRL | KEY_D:                  ed_trailing();       break;
	case MOD_CTRL | KEY_B:                  ed_opened();         break;
	case MOD_CTRL | KEY_I:                  ed_ins_include();    break;
	case MOD_CTRL | MOD_SHIFT | KEY_I:      ed_ins_include_lib();break;
	case MOD_CTRL | MOD_SHIFT | KEY_A:      ed_ins_comment();    break;
	case MOD_CTRL | KEY_A:                  ed_sel_all();        break;
	case MOD_CTRL | KEY_TAB:                ed_switch_buf();     break;
	default:
		if(isprint(cp) || cp == '\t')
		{
			ed_char(cp);
		}
		break;
	}

	assert(render_cnt <= 1);
}

static void ed_mode_default(void)
{
	mode = ED_MODE_DEFAULT;
	ed_render();
}

static void ed_key_press_msg(u32 key)
{
	if((msg_type == ED_ERROR && key == KEY_RETURN) ||
		(msg_type == ED_INFO))
	{
		ed_mode_default();
	}
}

static void ed_switch_to_buf(void)
{
	cur_buf = dir_pos;
	tb = ed_tb_get(cur_buf);
	ed_mode_default();
}

static void ed_save_buf(void)
{
}

/* FIXME ..... */
static void ed_buf_remove(u32 idx)
{
	/*vector_remove();*/
}

static void ed_discard_buf(void)
{
	tb_destroy(ed_tb_get(dir_pos));
	ed_buf_remove(dir_pos);
	if(dir_pos == dir_entries - 1)
	{
		--dir_pos;
	}

	--dir_entries;
	ed_render();
}

static void ed_key_press_opened(u32 key)
{
	switch(key)
	{
	case MOD_CTRL | KEY_B:
	case KEY_ESCAPE:             ed_mode_default();  break;
	case KEY_UP:                 nav_up();           break;
	case KEY_DOWN:               nav_down();         break;
	case KEY_PAGE_UP:            nav_page_up();      break;
	case KEY_PAGE_DOWN:          nav_page_down();    break;
	case KEY_HOME:               nav_first();        break;
	case KEY_END:                nav_last();         break;
	case KEY_RETURN:             ed_switch_to_buf(); break;
	case MOD_CTRL | KEY_S:       ed_save_buf();      break;
	case MOD_CTRL | KEY_DELETE:  ed_discard_buf();   break;
	}
}

static void ed_key_press(u32 key, u32 chr)
{
	switch(mode)
	{
	case ED_MODE_DEFAULT:
		ed_key_press_default(key, chr);
		break;

	case ED_MODE_NAV:
		ed_key_press_nav(key, chr);
		break;

	case ED_MODE_MSG:
		ed_key_press_msg(key);
		break;

	case ED_MODE_OPENED:
		ed_key_press_opened(key);
		break;
	}
}

static void event_keyboard(u32 key, u32 cp, u32 state)
{
	if(state == KEYSTATE_RELEASED)
	{
		return;
	}

	ed_key_press(key, cp);
}

static void event_resize(void)
{
	ed_render();
}

static void event_init(int argc, char *argv[])
{
#ifndef NDEBUG
	test_run_all();
#endif

	keyword_init(&c_hashmap);
	keyword_init(&asm_hashmap);
	ed_init();

	if(argc == 2)
	{
		ed_load(argv[1]);
	}

	ed_render();
}

static u32 event_exit(void)
{
	return ed_quit_internal();
}
