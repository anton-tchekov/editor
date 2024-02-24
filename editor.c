#include "helpers.c"
#include "keyword.c"
#include "test.c"
#include <string.h>

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

typedef struct
{
	u32 x, y;
} cursor;

typedef struct
{
	cursor c[2];
} selection;

typedef struct
{
	Vector lines;
	selection sel;
	char *filename;
	u32 page_y;
	i32 cursor_save_x;
	u8 language;
	u8 modified;
	u8 exists;
} text_buf;

static selection vsel;
static cursor vcursor;
static Vector buffers;

static u32 full_w, full_h, offset_x, page_w;

static u32 cur_buf;
static u32 untitled_cnt = 1;

static u32 tabsize;
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

static text_buf *tb;

static void ed_sel_to_cursor(void)
{
	tb->sel.c[0] = tb->sel.c[1];
}

static void cursor_swap(cursor *a, cursor *b)
{
	cursor temp = *a;
	*a = *b;
	*b = temp;
}

static u32 cursor_unordered(cursor *a, cursor *b)
{
	return (b->y < a->y) || (a->y == b->y && b->x < a->x);
}

static u32 sel_wide(selection *sel)
{
	return !(sel->c[0].x == sel->c[1].x && sel->c[0].y == sel->c[1].y);
}

static void sel_left(selection *sel)
{
	if(cursor_unordered(&sel->c[0], &sel->c[1]))
	{
		sel->c[0] = sel->c[1];
	}
	else
	{
		sel->c[1] = sel->c[0];
	}
}

static void sel_right(selection *sel)
{
	if(cursor_unordered(&sel->c[0], &sel->c[1]))
	{
		sel->c[1] = sel->c[0];
	}
	else
	{
		sel->c[0] = sel->c[1];
	}
}

static void ed_sel_norm(selection *sel)
{
	if(cursor_unordered(&sel->c[0], &sel->c[1]))
	{
		cursor_swap(&sel->c[0], &sel->c[1]);
	}
}

static text_buf *tb_get(u32 i)
{
	return *(text_buf **)vector_get(&buffers, i * sizeof(text_buf *));
}

static u32 ed_num_buffers(void)
{
	return vector_len(&buffers) / sizeof(text_buf *);
}

static u32 tb_num_lines(text_buf *t)
{
	return vector_len(&t->lines) / sizeof(Vector);
}

static u32 ed_num_lines(void)
{
	return tb_num_lines(tb);
}

static Vector *tb_line_get(text_buf *t, u32 i)
{
	return (Vector *)vector_get(&t->lines, i * sizeof(Vector));
}

static Vector *ed_line_get(u32 i)
{
	return tb_line_get(tb, i);
}

static Vector *ed_cur_line(void)
{
	return ed_line_get(tb->sel.c[1].y);
}

static u32 ed_line_len(u32 i)
{
	return vector_len(ed_line_get(i));
}

static const char *ed_line_data(u32 i)
{
	return vector_data(ed_line_get(i));
}

static u32 ed_cur_line_len(void)
{
	return vector_len(ed_cur_line());
}

static u32 ed_sel_count_bytes(selection *sel)
{
	u32 len;
	u32 x1 = sel->c[0].x;
	u32 y1 = sel->c[0].y;
	u32 x2 = sel->c[1].x;
	u32 y2 = sel->c[1].y;
	if(y1 == y2)
	{
		return x2 - x1;
	}

	len = ed_line_len(y1) - x1 + 1;
	for(++y1; y1 < y2; ++y1)
	{
		len += ed_line_len(y1) + 1;
	}

	return len + x2;
}

static char *ed_sel_get(selection *sel, u32 *out_len)
{
	u32 x1, y1, x2, y2, len;
	char *output, *p;
	selection nsel = *sel;
	ed_sel_norm(&nsel);

	x1 = nsel.c[0].x;
	y1 = nsel.c[0].y;
	x2 = nsel.c[1].x;
	y2 = nsel.c[1].y;

	*out_len = len = ed_sel_count_bytes(&nsel);
	p = output = _malloc(len + 1);
	if(y1 == y2)
	{
		memcpy(p, ed_line_data(y1) + x1, len);
		p += len;
	}
	else
	{
		Vector *line = ed_line_get(y1);
		len = vector_len(line) - x1;
		memcpy(p, (u8 *)vector_data(line) + x1, len);
		p += len;
		for(++y1; y1 < y2; ++y1)
		{
			*p++ = '\n';
			line = ed_line_get(y1);
			len = vector_len(line);
			memcpy(p, vector_data(line), len);
			p += len;
		}

		*p++ = '\n';
		memcpy(p, ed_line_data(y1), x2);
		p += x2;
	}

	*p = '\0';
	return output;
}

static void ed_remove_lines(u32 y1, u32 y2)
{
	u32 i;
	for(i = y1; i <= y2; ++i)
	{
		vector_destroy(ed_line_get(i));
	}

	vector_remove(&tb->lines, y1 * sizeof(Vector),
		(y2 - y1 + 1) * sizeof(Vector));
}

static void ed_sel_delete(void)
{
	u32 x1, y1, x2, y2;
	Vector *line;
	selection nsel = tb->sel;
	ed_sel_norm(&nsel);

	x1 = nsel.c[0].x;
	y1 = nsel.c[0].y;
	x2 = nsel.c[1].x;
	y2 = nsel.c[1].y;

	tb->sel.c[1] = tb->sel.c[0] = nsel.c[0];

	line = ed_line_get(y1);
	if(y1 == y2)
	{
		vector_remove(line, x1, x2 - x1);
	}
	else
	{
		Vector *last = ed_line_get(y2);
		vector_replace(line, x1, vector_len(line) - x1,
			(u8 *)vector_data(last) + x2, vector_len(last) - x2);
		ed_remove_lines(y1 + 1, y2);
	}
}

static void ed_sel_clear(void)
{
	if(sel_wide(&tb->sel))
	{
		ed_sel_delete();
	}
}

static void ed_ins_lines(u32 y, u32 count)
{
	vector_makespace(&tb->lines, y * sizeof(Vector), count * sizeof(Vector));
}

static void ed_insert(const char *text)
{
	u32 c, y, new_lines;
	const char *p, *s;

	ed_sel_clear();

	/* Count number of newlines and store position of last line */
	new_lines = 0;
	for(s = text; (c = *s); ++s)
	{
		if(c == '\n')
		{
			p = s + 1;
			++new_lines;
		}
	}

	y = tb->sel.c[1].y;
	if(!new_lines)
	{
		u32 len = s - text;
		vector_insert(ed_line_get(y), tb->sel.c[1].x, len, text);
		tb->sel.c[1].x += len;
	}
	else
	{
		char *last;
		u32 slen, rlen;
		Vector line, *first, *lines;

		/* Insert new lines in advance to avoid quadratic complexity */
		ed_ins_lines(y + 1, new_lines);
		lines = vector_data(&tb->lines);

		/* Last line */
		first = ed_line_get(y);
		rlen = vector_len(first) - tb->sel.c[1].x;
		slen = s - p;
		vector_init(&line, rlen + slen);
		line.len = rlen + slen;
		last = vector_data(&line);
		memcpy(last, p, slen);
		memcpy(last + slen, (u8 *)vector_data(first) + tb->sel.c[1].x, rlen);
		lines[y + new_lines] = line;

		first->len = tb->sel.c[1].x;
		tb->sel.c[1].x = slen;
		tb->sel.c[1].y = y + new_lines;

		/* First line */
		p = strchr(text, '\n');
		vector_push(ed_line_get(y), p - text, text);
		text = p + 1;

		/* Other lines */
		while((p = strchr(text, '\n')))
		{
			vector_from(&line, text, p - text);
			lines[++y] = line;
			text = p + 1;
		}
	}

	tb->cursor_save_x = -1;
	ed_sel_to_cursor();
	tb->modified = 1;
}

static void ed_line_remove(u32 line)
{
	vector_destroy(ed_line_get(line));
	vector_remove(&tb->lines, line * sizeof(Vector), sizeof(Vector));
}

static void ed_line_insert(u32 line, Vector *v)
{
	vector_insert(&tb->lines, line * sizeof(Vector), sizeof(Vector), v);
}

static void ed_render_linenr(u32 start_y, u32 end_y)
{
	u32 x, y, lnr_max, lnr_width;
	u32 lines = ed_num_lines();
	u32 lnr = tb->page_y + start_y;
	lnr_max = lnr + full_h;
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

	page_w = full_w - lnr_width - 1;
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
	Vector *lv = ed_line_get(y);
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

static u32 is_asm_ident(u32 c)
{
	return isalpha(c) || c == '.' || c == '_';
}

static u32 ed_asm6800(u32 y)
{
	Vector *lv = ed_line_get(y);
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
		else if(c == '(' || c == ')')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(isalpha(c) || c == '_')
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
	Vector *lv = ed_line_get(y);
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
		else if(c == '(' || c == ')')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '[' || c == ']')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '{' || c == '}')
		{
			if(x >= page_w) { return x; }
			ed_put(x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACE, COLOR_TABLE_BG)));
			++i;
		}
		else if(isalpha(c) || c == '_')
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
	if(line < ed_num_lines())
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
	u32 y = full_h - 1;
	u32 color = screen_color(COLOR_TABLE_FG,
		msg_type ? COLOR_TABLE_ERROR : COLOR_TABLE_INFO);

	for(x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, screen_pack(c, color));
	}

	for(; x < full_w; ++x)
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
		for(; *p && x < full_w; ++p, ++x)
		{
			screen_set(x, y, screen_pack(*p, color));
		}

		for(; x < full_w; ++x)
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
		text_buf *t = tb_get(i);
		u32 color = (i == dir_pos) ?
			screen_color(COLOR_TABLE_ERROR, COLOR_TABLE_GRAY) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_GRAY);

		u32 x = 0;
		const char *p = t->filename;
		screen_set(x++, y, screen_pack(t->modified ? '*' : ' ', color));
		for(; *p && x < full_w; ++p, ++x)
		{
			screen_set(x, y, screen_pack(*p, color));
		}

		for(; x < full_w; ++x)
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

	for(s = nav_buf, i = 0; x < full_w; ++x, ++s, ++i)
	{
		screen_set(x, 0, screen_pack((i < nav_len) ? *s : ' ',
			(i == nav_cursor) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG)));
	}

	return ed_render_dir();
}

static u32 ed_chr_x_inc(u32 c, u32 x)
{
	return (c == '\t') ?
		((x + tabsize) / tabsize * tabsize) :
		(x + 1);
}

static u32 ed_cursor_pos_x(u32 y, u32 end)
{
	u32 i;
	u32 x = 0;
	const char *line = vector_data(ed_line_get(y));
	for(i = 0; i < end; ++i)
	{
		x = ed_chr_x_inc(line[i], x);
	}

	return x;
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
		Vector *line = ed_line_get(i);
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
	u32 end_y = full_h;
	u32 lines = ed_num_lines();

	if(tb->sel.c[1].y < tb->page_y)
	{
		tb->page_y = tb->sel.c[1].y;
	}

	if(tb->sel.c[1].y >= tb->page_y + full_h)
	{
		tb->page_y = tb->sel.c[1].y - full_h + 1;
	}

	if(lines > full_h && tb->page_y + full_h >= lines)
	{
		tb->page_y = lines - full_h;
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

	vcursor.x = ed_cursor_pos_x(tb->sel.c[1].y, tb->sel.c[1].x);
	vcursor.y = tb->sel.c[1].y;
	vsel.c[0] = vcursor;
	if(tb->sel.c[0].y == tb->sel.c[1].y && tb->sel.c[1].x == tb->sel.c[0].x)
	{
		vsel.c[1] = vcursor;
	}
	else
	{
		vsel.c[1].x = ed_cursor_pos_x(tb->sel.c[0].y, tb->sel.c[0].x);
		vsel.c[1].y = tb->sel.c[0].y;
	}

	ed_sel_norm(&vsel);
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
		page_w = full_w;
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
	if(sel_wide(&tb->sel))
	{
		ed_sel_delete();
	}
	else
	{
		Vector *line = ed_cur_line();
		if(tb->sel.c[1].x == 0)
		{
			if(tb->sel.c[1].y > 0)
			{
				/* Merge with previous line */
				Vector *prev = ed_line_get(--tb->sel.c[1].y);
				tb->sel.c[1].x = vector_len(prev);
				vector_push(prev, vector_len(line), vector_data(line));
				ed_line_remove(tb->sel.c[1].y + 1);
			}
		}
		else
		{
			/* Delete previous char */
			vector_remove(line, --tb->sel.c[1].x, 1);
		}

		tb->cursor_save_x = -1;
		ed_sel_to_cursor();
	}

	ed_render();
}

static void ed_delete(void)
{
	if(sel_wide(&tb->sel))
	{
		ed_sel_delete();
	}
	else
	{
		Vector *line = ed_cur_line();
		u32 line_len = vector_len(line);
		if(tb->sel.c[1].x >= line_len)
		{
			u32 num_lines = ed_num_lines();
			tb->sel.c[1].x = line_len;
			if(tb->sel.c[1].y < num_lines - 1)
			{
				/* Merge with next line */
				u32 next_idx = tb->sel.c[1].y + 1;
				Vector *next = ed_line_get(next_idx);
				vector_push(line, vector_len(next), vector_data(next));
				ed_line_remove(next_idx);
			}
		}
		else
		{
			/* Delete next char */
			vector_remove(line, tb->sel.c[1].x, 1);
		}

		tb->cursor_save_x = -1;
	}

	ed_render();
}

static void ed_char(u32 chr)
{
	u8 ins[1];
	ed_sel_clear();
	ins[0] = chr;
	vector_insert(ed_cur_line(), tb->sel.c[1].x, 1, ins);
	++tb->sel.c[1].x;
	tb->cursor_save_x = -1;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_enter(void)
{
	u32 len;
	char *str;
	Vector new_line, *cur;

	ed_sel_clear();

	cur = ed_cur_line();
	str = (char *)vector_data(cur) + tb->sel.c[1].x;
	len = vector_len(cur) - tb->sel.c[1].x;

	/* Copy characters after cursor on current line to new line */
	vector_init(&new_line, len);
	vector_push(&new_line, len, str);

	/* Insert new line */
	ed_line_insert(tb->sel.c[1].y + 1, &new_line);

	/* Remove characters after cursor on current line */
	vector_remove(cur, tb->sel.c[1].x, len);

	++tb->sel.c[1].y;
	tb->sel.c[1].x = 0;
	tb->cursor_save_x = 0;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_enter_after(void)
{
	Vector new_line;
	vector_init(&new_line, 8);
	ed_line_insert(++tb->sel.c[1].y, &new_line);
	tb->sel.c[1].x = 0;
	tb->cursor_save_x = 0;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_enter_before(void)
{
	Vector new_line;
	vector_init(&new_line, 8);
	ed_line_insert(tb->sel.c[1].y, &new_line);
	tb->sel.c[1].x = 0;
	tb->cursor_save_x = 0;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_home_internal(void)
{
	Vector *line = ed_cur_line();
	char *buf = vector_data(line);
	u32 len = vector_len(line);
	u32 i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	tb->sel.c[1].x = (tb->sel.c[1].x == i) ? 0 : i;
	tb->cursor_save_x = -1;
}

static void ed_sel_home(void)
{
	ed_home_internal();
	ed_render();
}

static void ed_home(void)
{
	ed_home_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_end_internal(void)
{
	tb->sel.c[1].x = ed_cur_line_len();
	tb->cursor_save_x = -1;
}

static void ed_sel_end(void)
{
	ed_end_internal();
	ed_render();
}

static void ed_end(void)
{
	ed_end_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_move_vertical(u32 prev_y)
{
	Vector *line = ed_cur_line();
	u32 len = vector_len(line);
	const char *buf = vector_data(line);
	u32 i, x, max_x;

	if(tb->cursor_save_x < 0)
	{
		tb->cursor_save_x = ed_cursor_pos_x(prev_y, tb->sel.c[1].x);
	}

	max_x = tb->cursor_save_x;
	for(i = 0, x = 0; i < len && x < max_x; ++i)
	{
		x = ed_chr_x_inc(buf[i], x);
	}

	tb->sel.c[1].x = i;
}

static void ed_up_internal(void)
{
	if(tb->sel.c[1].y == 0)
	{
		tb->sel.c[1].x = 0;
		tb->cursor_save_x = 0;
	}
	else
	{
		u32 prev_y = tb->sel.c[1].y;
		--tb->sel.c[1].y;
		ed_move_vertical(prev_y);
	}
}

static void ed_sel_up(void)
{
	ed_up_internal();
	ed_render();
}

static void ed_up(void)
{
	ed_up_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_down_internal(void)
{
	if(tb->sel.c[1].y >= ed_num_lines() - 1)
	{
		tb->sel.c[1].x = ed_cur_line_len();
		tb->cursor_save_x = -1;
	}
	else
	{
		u32 prev_y = tb->sel.c[1].y;
		++tb->sel.c[1].y;
		ed_move_vertical(prev_y);
	}
}

static void ed_sel_down(void)
{
	ed_down_internal();
	ed_render();
}

static void ed_down(void)
{
	ed_down_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_page_up_internal(void)
{
	if(tb->sel.c[1].y >= full_h)
	{
		u32 prev_y = tb->sel.c[1].y;
		tb->sel.c[1].y -= full_h;
		ed_move_vertical(prev_y);
	}
	else
	{
		tb->sel.c[1].y = 0;
		tb->sel.c[1].x = 0;
		tb->cursor_save_x = 0;
	}
}

static void ed_sel_page_up(void)
{
	ed_page_up_internal();
	ed_render();
}

static void ed_page_up(void)
{
	ed_page_up_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_page_down_internal(void)
{
	u32 num_lines = ed_num_lines();
	u32 prev_y = tb->sel.c[1].y;
	tb->sel.c[1].y += full_h;
	if(tb->sel.c[1].y >= num_lines)
	{
		tb->sel.c[1].y = num_lines - 1;
		tb->sel.c[1].x = ed_cur_line_len();
		tb->cursor_save_x = -1;
	}
	else
	{
		ed_move_vertical(prev_y);
	}
}

static void ed_sel_page_down(void)
{
	ed_page_down_internal();
	ed_render();
}

static void ed_page_down(void)
{
	ed_page_down_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_left_internal(void)
{
	if(tb->sel.c[1].x == 0)
	{
		if(tb->sel.c[1].y > 0)
		{
			--tb->sel.c[1].y;
			tb->sel.c[1].x = ed_cur_line_len();
		}
	}
	else
	{
		--tb->sel.c[1].x;
	}

	tb->cursor_save_x = -1;
}

static void ed_sel_left(void)
{
	ed_left_internal();
	ed_render();
}

static void ed_left(void)
{
	if(sel_wide(&tb->sel))
	{
		sel_left(&tb->sel);
	}
	else
	{
		ed_left_internal();
		ed_sel_to_cursor();
	}

	ed_render();
}

static void ed_right_internal(void)
{
	if(tb->sel.c[1].x == ed_cur_line_len())
	{
		if(tb->sel.c[1].y < ed_num_lines() - 1)
		{
			++tb->sel.c[1].y;
			tb->sel.c[1].x = 0;
		}
	}
	else
	{
		++tb->sel.c[1].x;
	}

	tb->cursor_save_x = -1;
}

static void ed_sel_right(void)
{
	ed_right_internal();
	ed_render();
}

static void ed_right(void)
{
	if(sel_wide(&tb->sel))
	{
		sel_right(&tb->sel);
	}
	else
	{
		ed_right_internal();
		ed_sel_to_cursor();
	}

	ed_render();
}

static void ed_top_internal(void)
{
	tb->sel.c[1].x = 0;
	tb->sel.c[1].y = 0;
	tb->cursor_save_x = 0;
}

static void ed_sel_top(void)
{
	ed_top_internal();
	ed_render();
}

static void ed_top(void)
{
	ed_top_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_bottom_internal(void)
{
	tb->sel.c[1].y = ed_num_lines() - 1;
	tb->sel.c[1].x = ed_cur_line_len();
	tb->cursor_save_x = -1;
}

static void ed_sel_bottom(void)
{
	ed_bottom_internal();
	ed_render();
}

static void ed_bottom(void)
{
	ed_bottom_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_reset_cursor(void)
{
	tb->page_y = 0;
	tb->cursor_save_x = 0;
	memset(&tb->sel, 0, sizeof(selection));
}

static void ed_gotoxy(u32 x, u32 y)
{
	u32 num_lines = ed_num_lines();
	if(y >= num_lines)
	{
		y = num_lines - 1;
	}

	tb->sel.c[1].y = y;
	if(tb->sel.c[1].y > full_h / 2)
	{
		tb->page_y = tb->sel.c[1].y - full_h / 2;
	}

	tb->sel.c[1].x = x;
	ed_sel_to_cursor();
	tb->cursor_save_x = -1;
}

static i32 str_find(const char *haystack, u32 len, const char *needle, u32 sl)
{
	u32 i;
	if(len < sl)
	{
		return -1;
	}

	for(i = 0; i < len - sl; ++i)
	{
		if(!memcmp(haystack + i, needle, sl))
		{
			return i;
		}
	}

	return -1;
}

static void ed_goto_def(const char *s)
{
	u32 i;
	u32 len = ed_num_lines();
	u32 sl = strlen(s);
	for(i = 0; i < len; ++i)
	{
		Vector *line = ed_line_get(i);
		u32 ll = vector_len(line);
		const char *buf = vector_data(line);
		i32 offset;

		if(ll == 0 || isspace(buf[0]))
		{
			continue;
		}

		if((offset = str_find(buf, ll, s, sl)) < 0)
		{
			continue;
		}

		ed_gotoxy(offset, i);
		return;
	}
}

static u32 ed_switch_to(const char *fname)
{
	u32 i, len = ed_num_buffers();
	for(i = 0; i < len; ++i)
	{
		text_buf *cur = tb_get(i);
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
	u32 c;
	Vector line;
	const char *linestart, *p;
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

	mode = ED_MODE_DEFAULT;
	tb = _malloc(sizeof(text_buf));
	vector_push(&buffers, sizeof(text_buf *), &tb);
	vector_init(&tb->lines, count_char(buf, '\n') + 1);
	tb->filename = _malloc(strlen(filename));
	strcpy(tb->filename, filename);
	tb->language = LANGUAGE_C;
	tb->modified = 0;
	tb->exists = 1;
	ed_reset_cursor();
	p = buf;
	linestart = buf;
	do
	{
		c = *p;
		if(c == '\0' || c == '\n')
		{
			vector_from(&line, linestart, p - linestart);
			vector_push(&tb->lines, sizeof(line), &line);
			linestart = p + 1;
		}
		++p;
	}
	while(c != '\0');
	_free(buf);
}

static void tb_init(void)
{
	Vector line;
	tb = _malloc(sizeof(text_buf));
	vector_push(&buffers, sizeof(text_buf *), &tb);

	vector_init(&tb->lines, 32 * sizeof(Vector));
	vector_init(&line, 8);
	vector_push(&tb->lines, sizeof(line), &line);

	ed_reset_cursor();
	tb->language = LANGUAGE_C;
	tb->modified = 0;
	tb->exists = 0;
	tb->filename = _malloc(32);
	sprintf(tb->filename, "untitled-%d", untitled_cnt++);
}

static void tb_destroy(text_buf *t)
{
	u32 i, num_lines = tb_num_lines(t);
	for(i = 0; i < num_lines; ++i)
	{
		vector_destroy(tb_line_get(t, i));
	}

	vector_destroy(&t->lines);
	_free(t->filename);
	_free(t);
}

static void ed_new_file(void)
{
	tb_init();
	ed_render();
}

static void ed_init(u32 width, u32 height)
{
	vector_init(&buffers, 8 * sizeof(text_buf *));
	tb_init();

	tabsize = 4;
	show_whitespace = 1;
	full_w = width;
	full_h = height;
	show_linenr = 1;

	strcpy(nav_base, "./");
	nav_buf = nav_base + 2;
}

static void ed_sel_store(void)
{
	u32 len;
	char *text = ed_sel_get(&tb->sel, &len);
	clipboard_store(text);
	_free(text);
}

static void ed_sel_all(void)
{
	u32 last_line = ed_num_lines() - 1;
	tb->sel.c[0].x = 0;
	tb->sel.c[0].y = 0;
	tb->sel.c[1].x = ed_line_len(last_line);
	tb->sel.c[1].y = last_line;
	ed_render();
}

static void ed_cut(void)
{
	ed_sel_store();
	ed_sel_delete();
	ed_render();
}

static void ed_copy(void)
{
	ed_sel_store();
}

static void ed_paste(void)
{
	char *text = clipboard_load();
	ed_insert(text);
	free(text);
	ed_render();
}

static void ed_prev_word_internal(void)
{
	if(tb->sel.c[1].x == 0)
	{
		if(tb->sel.c[1].y > 0)
		{
			--tb->sel.c[1].y;
			tb->sel.c[1].x = ed_cur_line_len();
		}
	}
	else
	{
		if(tb->sel.c[1].x > 0)
		{
			char *buf = vector_data(ed_cur_line());
			u32 i = tb->sel.c[1].x - 1;
			u32 type = char_type(buf[i]);
			while(i > 0 && char_type(buf[i - 1]) == type) { --i; }
			tb->sel.c[1].x = i;
		}
	}

	tb->cursor_save_x = -1;
}

static void ed_sel_prev_word(void)
{
	ed_prev_word_internal();
	ed_render();
}

static void ed_prev_word(void)
{
	ed_prev_word_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_del_prev_word(void)
{
	ed_prev_word_internal();
	ed_sel_clear();
	tb->modified = 1;
	ed_render();
}

static void ed_next_word_internal(void)
{
	u32 len = ed_cur_line_len();
	if(tb->sel.c[1].x == len)
	{
		if(tb->sel.c[1].y < ed_num_lines() - 1)
		{
			++tb->sel.c[1].y;
			tb->sel.c[1].x = 0;
		}
	}
	else
	{
		if(tb->sel.c[1].x < len)
		{
			char *buf = vector_data(ed_cur_line());
			u32 i = tb->sel.c[1].x;
			u32 type = char_type(buf[i]);
			while(i < len && char_type(buf[i]) == type) { ++i; }
			tb->sel.c[1].x = i;
		}
	}

	tb->cursor_save_x = -1;
}

static void ed_sel_next_word(void)
{
	ed_next_word_internal();
	ed_render();
}

static void ed_next_word(void)
{
	ed_next_word_internal();
	ed_sel_to_cursor();
	ed_render();
}

static void ed_del_next_word(void)
{
	ed_next_word_internal();
	ed_sel_clear();
	tb->modified = 1;
	ed_render();
}

static void ed_del_cur_line(void)
{
	u32 count = ed_num_lines();
	if(count == 1)
	{
		ed_line_get(0)->len = 0;
	}
	else
	{
		ed_line_remove(tb->sel.c[1].y);
		if(tb->sel.c[1].y >= count - 1)
		{
			tb->sel.c[1].y = count - 2;
		}
	}

	tb->sel.c[1].x = 0;
	tb->cursor_save_x = 0;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_move_up(void)
{
	if(tb->page_y > 0)
	{
		--tb->page_y;
	}

	ed_render();
}

static void ed_move_down(void)
{
	++tb->page_y;
	ed_render();
}

static void ed_toggle_line_nr(void)
{
	show_linenr = !show_linenr;
	ed_render();
}

static void ed_toggle_lang(void)
{
	if(++tb->language == LANGUAGE_COUNT)
	{
		tb->language = 0;
	}
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

static u32 ed_count_bytes(void)
{
	u32 i, len, num_lines = ed_num_lines();
	for(i = 0, len = 0; i < num_lines; ++i)
	{
		len += ed_line_len(i) + 1;
	}

	return len - 1;
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
		u32 len = ed_count_bytes();
		char *buf = _malloc(len);
		char *p = buf;
		u32 i, num_lines = ed_num_lines();
		for(i = 0; i < num_lines; ++i)
		{
			Vector *cur = ed_line_get(i);
			u32 line_len = vector_len(cur);
			memcpy(p, vector_data(cur), line_len);
			p += line_len;
			if(i < num_lines - 1)
			{
				*p++ = '\n';
			}
		}

		if(file_write(tb->filename, buf, len))
		{
			ed_msg(ED_ERROR, "Writing file failed");
		}
		else
		{
			ed_msg(ED_INFO, "File saved");
		}

		_free(buf);
		tb->modified = 0;
	}
	else
	{
		ed_save_as();
	}
}

static void ed_ins(const char *s, u32 len, u32 inc)
{
	ed_sel_clear();
	vector_insert(ed_cur_line(), tb->sel.c[1].x, len, s);
	tb->sel.c[1].x += inc;
	tb->cursor_save_x = -1;
	ed_sel_to_cursor();
	tb->modified = 1;
	ed_render();
}

static void ed_ins_comment(void)
{
	static const char *ins = "/*  */";
	ed_ins(ins, 6, 3);
}

static void ed_include(void)
{
	static const char *ins = "#include \"\"";
	ed_ins(ins, 11, 10);
}

static void ed_include_lib(void)
{
	static const char *ins = "#include <>";
	ed_ins(ins, 11, 10);
}

static void ed_sel_cur_line(void)
{
	u32 last_line = ed_num_lines() - 1;
	tb->sel.c[0].x = 0;
	if(tb->sel.c[1].y < last_line)
	{
		tb->sel.c[1].x = 0;
		++tb->sel.c[1].y;
	}
	else
	{
		u32 last_line_len = ed_line_len(last_line);
		if(tb->sel.c[1].x == last_line_len)
		{
			return;
		}

		tb->sel.c[1].x = last_line_len;
	}

	ed_render();
}

static void ed_trailing(void)
{
	u32 i, len, end = ed_num_lines();
	for(i = 0; i < end; ++i)
	{
		Vector *line = ed_line_get(i);
		char *data = vector_data(line);
		len = vector_len(line);
		while(len > 0 && isspace(data[len - 1]))
		{
			--len;
		}

		if(len != line->len)
		{
			tb->modified = 1;
			line->len = len;
		}
	}

	len = ed_line_len(tb->sel.c[0].y);
	if(tb->sel.c[0].x > len)
	{
		tb->sel.c[0].x = len;
	}

	len = ed_line_len(tb->sel.c[1].y);
	if(tb->sel.c[1].x > len)
	{
		tb->cursor_save_x = -1;
		tb->sel.c[1].x = len;
	}

	ed_render();
}

static void ed_close_file(void)
{
	if(tb->modified)
	{

	}
}

static u32 ed_has_unsaved(void)
{
	u32 i, len = ed_num_buffers();
	for(i = 0; i < len; ++i)
	{
		if(tb_get(i)->modified)
		{
			return 1;
		}
	}

	return 0;
}

static void ed_cleanup(void)
{
	u32 i, len = ed_num_buffers();
	for(i = 0; i < len; ++i)
	{
		tb_destroy(tb_get(i));
	}

	vector_destroy(&buffers);
	_free(dir_list);
}

static void ed_mode_opened(void)
{
	mode = ED_MODE_OPENED;
	dir_pos = 0;
	dir_offset = 0;
	dir_entries = ed_num_buffers();
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
	u32 cnt = ed_num_buffers();
	if(cnt == 1)
	{
		return;
	}

	++cur_buf;
	if(cur_buf == cnt)
	{
		cur_buf = 0;
	}

	tb = tb_get(cur_buf);
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
	case MOD_CTRL | KEY_I:                  ed_include();        break;
	case MOD_CTRL | MOD_SHIFT | KEY_I:      ed_include_lib();    break;
	case MOD_CTRL | KEY_A:                  ed_sel_all();        break;
	case MOD_CTRL | MOD_SHIFT | KEY_A:      ed_ins_comment();    break;
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
	tb = tb_get(cur_buf);
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
	tb_destroy(tb_get(dir_pos));
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
	if(key == (MOD_CTRL | MOD_SHIFT | KEY_Q))
	{
		fprintf(stderr, "Force EXIT\n");
		exit(1);
	}

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

static void event_resize(u32 w, u32 h)
{
	full_w = w;
	full_h = h;
	ed_render();
}

static void event_init(int argc, char *argv[], u32 w, u32 h)
{
	char buf[1024];

#ifndef NDEBUG
	test_run_all();
#endif

	get_working_dir(buf);
	printf("%s\n", buf);

	keyword_init(&c_hashmap);
	keyword_init(&asm_hashmap);
	ed_init(w, h);

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
