#include "helpers.c"
#include "keyword.c"
#include "test.c"
#include <string.h>

enum
{
	EDITOR_MODE_DEFAULT,
	EDITOR_MODE_GOTO,
	EDITOR_MODE_MSG,
	EDITOR_MODE_COUNT
};

enum
{
	EDITOR_INFO,
	EDITOR_ERROR
};

#define MAX_SEARCH_LEN    256
#define MAX_MSG_LEN        80
#define ED_DIR_BUF_SIZE  8192
#define ED_DIR_PAGE        12

typedef struct
{
	Vector Lines;
	u32 CursorX;
	u32 CursorSaveX;
	u32 CursorY;
	u32 PageW;
	u32 FullW;
	u32 OffsetX;
	u32 PageH;
	u32 PageX;
	u32 PageY;
	u32 TabSize;
	u8 ShowLineNr;
	u8 ShowWhitespace;
	u8 Mode;

	u8 MsgType;
	char Msg[MAX_MSG_LEN];

	char FileName[128];

	char SBuf[MAX_SEARCH_LEN];
	char *Search;
	u32 SCursor, SLen;

	u8 FirstCompare;
	char *SearchFile;
	char Same[64];

	char DirBuf[ED_DIR_BUF_SIZE];
	u32 DirOverflow, DirEntries, DirIdx, DirOffset, DirPos;
} Editor;

typedef void (*EditorFunc)(Editor *);

typedef struct
{
	u16 Key;
	EditorFunc Event;
} EditorKeyBind;

#if 0

typedef struct
{
	size_t X, Y;
} Cursor;

typedef struct
{
	Cursor Limits[2];
} Selection;

static void cursor_normalize(TextBuffer *tb, Cursor *cursor)
{
	size_t length = tb->Lines[cursor->Y]->Length;
	cursor->X = (cursor->X < length) ? cursor->X : length;
}

static void cursor_swap(Cursor *a, Cursor *b)
{
	Cursor temp = *a;
	*a = *b;
	*b = temp;
}

static int cursor_unordered(Cursor *a, Cursor *b)
{
	return (b->Y < a->Y) || (a->Y == b->Y && b->X < a->X);
}

static void selection_normalize(TextBuffer *tb, Selection *sel)
{
	Cursor *first = &sel->Limits[0];
	Cursor *second = &sel->Limits[1];

	cursor_normalize(tb, first);
	cursor_normalize(tb, second);

	if(cursor_unordered(first, second))
	{
		cursor_swap(first, second);
	}
}

static size_t textline_col(const TextLine *tl, size_t col, size_t tabsize)
{
	size_t x;
	const char *p, *e;
	p = tl->Buffer;
	e = p + tl->Length;
	for(; p < e; ++p)
	{
		if(*p == '\t')
		{
			x = x + tabsize - (x & (tabsize - 1));
		}
		else
		{
			++x;
		}
	}

	return x;
}

static size_t count_char(const char *text, size_t len, int c)
{
	size_t count = 0;
	const char *p = text;
	const char *end = text + len;
	while((p = memchr(p, c, len)))
	{
		++p;
		len = end - p;
		++count;
	}

	return count;
}

static void textbuffer_insert_lines(TextBuffer *tb, size_t y1, size_t y2,
	size_t ins_lines)
{
	size_t rem_lines = y2 - y1;
	if(ins_lines != rem_lines)
	{
		textbuffer_require(tb, tb->Capacity + ins_lines - rem_lines);
		/* FIXME: MEMORY LEAK !!! */
		memmove(tb->Lines + y1 + ins_lines, tb->Lines + y2,
			(tb->Count - y2) * sizeof(TextLine *));
	}
}

static size_t count_bytes(TextBuffer *tb, const Selection *sel)
{
	size_t length;
	size_t x1 = sel->Limits[0].X;
	size_t y1 = sel->Limits[0].Y;
	size_t x2 = sel->Limits[1].X;
	size_t y2 = sel->Limits[1].Y;
	TextLine **lines = tb->Lines;
	TextLine **cur = lines + y1;
	TextLine **end = lines + y2;

	if(y1 == y2)
	{
		length = x2 - x1;
	}
	else
	{
		length = (*cur)->Length - x1 + 1;
		++cur;
		while(cur < end)
		{
			length += (*cur)->Length + 1;
			++cur;
		}

		length += x2;
	}

	return length;
}

static char *textbuffer_get(TextBuffer *tb, const Selection *sel, size_t *len)
{
	TextLine **lines, **cur, **end;
	Selection nsel = *sel;
	size_t x1, y1, x2, y2;
	size_t length;
	char *output, *p;

	selection_normalize(tb, &nsel);

	x1 = nsel.Limits[0].X;
	y1 = nsel.Limits[0].Y;
	x2 = nsel.Limits[1].X;
	y2 = nsel.Limits[1].Y;

	length = count_bytes(tb, &nsel);
	output = _malloc(length + 1);
	*len = length;

	lines = tb->Lines;
	cur = lines + y1;
	end = lines + y2;

	p = output;
	if(y1 == y2)
	{
		/* Copy span of single line */
		memcpy(p, (*cur)->Buffer + x1, length);
		p += length;
	}
	else
	{
		/* Copy end of first line */
		length = (*cur)->Length - x1;
		memcpy(p, (*cur)->Buffer + x1, length);
		p += length;
		++cur;

		/* Copy all lines in between */
		while(cur < end)
		{
			*p++ = '\n';
			length = (*cur)->Length;
			memcpy(p, (*cur)->Buffer, length);
			p += length;
			++cur;
		}

		/* Copy start of last line */
		*p++ = '\n';
		memcpy(p, (*cur)->Buffer, x2);
		p += x2;
	}

	*p = '\0';
	return output;
}

#endif

static u32 ed_num_lines(Editor *ed)
{
	return vector_len(&ed->Lines) / sizeof(Vector);
}

static Vector *ed_line_get(Editor *ed, u32 i)
{
	return (Vector *)vector_get(&ed->Lines, i * sizeof(Vector));
}

static Vector *ed_cur_line(Editor *ed)
{
	return ed_line_get(ed, ed->CursorY);
}

static u32 ed_line_len(Editor *ed, u32 i)
{
	return vector_len(ed_line_get(ed, i));
}

static u32 ed_cur_line_len(Editor *ed)
{
	return vector_len(ed_cur_line(ed));
}

static void ed_line_remove(Editor *ed, u32 line)
{
	vector_remove(&ed->Lines, line * sizeof(Vector), sizeof(Vector));
}

static void ed_line_insert(Editor *ed, u32 line, Vector *v)
{
	vector_insert(&ed->Lines, line * sizeof(Vector), sizeof(Vector), v);
}

static void ed_render_linenr(Editor *ed)
{
	u32 x, y, lnr_max, lnr_width;

	u32 def = screen_color(COLOR_TABLE_LINE_NUMBER, COLOR_TABLE_BG);
	u32 cur = screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG);

	u32 lines = ed_num_lines(ed);
	u32 lnr = ed->PageY;
	lnr_max = lnr + ed->PageH;
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);

	for(y = 0; y < ed->PageH; ++y)
	{
		u32 color = (lnr == ed->CursorY) ? cur : def;
		++lnr;
		if(lnr <= lines)
		{
			char lnr_buf[8];
			linenr_str(lnr_buf, lnr, lnr_width);
			for(x = 0; x < lnr_width; ++x)
			{
				screen_set(x, y, lnr_buf[x], color);
			}
		}
		else
		{
			for(x = 0; x < lnr_width; ++x)
			{
				screen_set(x, y, ' ', color);
			}
		}

		screen_set(x, y, ' ', color);
	}

	ed->PageW = ed->FullW - lnr_width - 1;
	ed->OffsetX = lnr_width + 1;
}

static u32 ed_syntax(Editor *ed, u32 len, const char *line, u16 *render)
{
	u32 c;
	u32 incflag = 0;
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		c = line[i];
		if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				render[x++] = screen_pack(line[i],
					screen_color(COLOR_TABLE_COMMENT, COLOR_TABLE_BG));
			}
		}
		else if(c == ' ' && ed->ShowWhitespace)
		{
			render[x++] = screen_pack(CHAR_VISIBLE_SPACE,
				screen_color(COLOR_TABLE_VISIBLE_SPACE, COLOR_TABLE_BG));
			++i;
		}
		else if(c == '\t')
		{
			u32 n = x & (ed->TabSize - 1);
			if(ed->ShowWhitespace)
			{
				u32 color = screen_color(COLOR_TABLE_VISIBLE_SPACE, COLOR_TABLE_BG);
				if(n == ed->TabSize - 1)
				{
					render[x++] = screen_pack(CHAR_TAB_BOTH, color);
				}
				else
				{
					render[x++] = screen_pack(CHAR_TAB_START, color);
					for(++n; n < ed->TabSize - 1; ++n)
					{
						render[x++] = screen_pack(CHAR_TAB_MIDDLE, color);
					}
					render[x++] = screen_pack(CHAR_TAB_END, color);
				}
			}
			else
			{
				u32 color = screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG);
				for(; n < ed->TabSize; ++n)
				{
					render[x++] = screen_pack(' ', color);
				}
			}

			++i;
		}
		else if(incflag && c == '<')
		{
			for(; i < len; ++i)
			{
				c = line[i];
				render[x++] = screen_pack(c,
					screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG));
				if(c == '>')
				{
					++i;
					break;
				}
			}
		}
		else if(c == '#')
		{
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG));

			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				render[x++] = screen_pack(c,
					screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG));
			}

			incflag = 1;
		}
		else if(c == '\"' || c == '\'')
		{
			u32 save = c;
			u32 esc = 0;
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG));

			for(++i; i < len; ++i)
			{
				c = line[i];
				render[x++] = screen_pack(c,
					screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG));

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
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG));
			++i;
		}
		else if(c == '[' || c == ']')
		{
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG));
			++i;
		}
		else if(c == '{' || c == '}')
		{
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_BRACE, COLOR_TABLE_BG));
			++i;
		}
		else if(isalpha(c) || c == '_')
		{
			u32 color, end, start;
			for(start = i; i < len && (is_ident(c = line[i])); ++i) {}
			end = i;
			color = screen_color(
				keyword_detect(line + start, end - start), COLOR_TABLE_BG);

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
				render[x++] = screen_pack(line[i], color);
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
						/* Hex */
						for(++e; e < len && isxdigit(line[e]); ++e) {}
					}
					else if(c == 'b' || c == 'B')
					{
						/* Binary */
						for(++e; e < len && is_bin(line[e]); ++e) {}
					}
					else
					{
						/* Octal */
						for(; e < len && is_oct(line[e]); ++e) {}
					}
				}
			}
			else
			{
				/* Decimal */
				for(; e < len && isdigit(line[e]); ++e) {}
			}

			for(; i < e; ++x, ++i)
			{
				render[x] = screen_pack(line[i],
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
			}
		}
		else
		{
			render[x++] = screen_pack(c,
				screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG));
			++i;
		}
	}

	return x;
}

static u32 cursor_pos_x(Editor *ed)
{
	u32 i;
	u32 r = 0;
	u32 x = ed->CursorX - ed->PageX;
	const char *line = vector_data(ed_cur_line(ed));
	for(i = 0; i < x; ++i)
	{
		if(line[i] == '\t')
		{
			r = (r + ed->TabSize) / ed->TabSize * ed->TabSize;
		}
		else
		{
			++r;
		}
	}

	return r;
}

static void ed_render_line(Editor *ed, u32 y, u32 cursor_x)
{
	u32 x, line, len, render_len, pack;
	u16 render[1024]; /* FIXME: Crash when line is longer than 256 chars */
	const char *text;

	line = ed->PageY + y;
	if(line >= ed_num_lines(ed))
	{
		len = 0;
	}
	else
	{
		Vector *lv = ed_line_get(ed, line);
		len = vector_len(lv);
		text = vector_data(lv);
	}

	pack = screen_pack(' ', screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG));
	render_len = ed_syntax(ed, len, text, render);
	render[render_len++] = pack;
	if(line == ed->CursorY)
	{
		render[cursor_x] = screen_pack_color_swap(render[cursor_x]);
	}

	for(x = 0; x < ed->PageW && x < render_len; ++x)
	{
		screen_set_pack(x + ed->OffsetX, y, render[x]);
	}

	for(; x < ed->PageW; ++x)
	{
		screen_set_pack(x + ed->OffsetX, y, pack);
	}
}

static void ed_render_msg(Editor *ed)
{
	const char *s = ed->Msg;
	u32 x, c;
	u32 y = ed->PageH - 1;
	u32 color = screen_color(COLOR_TABLE_FG,
		ed->MsgType ? COLOR_TABLE_ERROR : COLOR_TABLE_INFO);

	for(x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, c, color);
	}

	for(; x < ed->FullW; ++x)
	{
		screen_set(x, y, ' ', color);
	}
}

static u32 ed_render_dir(Editor *ed)
{
	u32 i, x;
	u32 y = 1;
	const char *p = ed->DirBuf + ed->DirIdx;
	u32 end = ed->DirOffset + ED_DIR_PAGE;
	if(end > ed->DirEntries)
	{
		end = ed->DirEntries;
	}

	for(i = ed->DirOffset; i < end; ++i, ++p, ++y)
	{
		u32 color = (i == ed->DirPos) ?
			screen_color(COLOR_TABLE_INFO, COLOR_TABLE_VISIBLE_SPACE) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_VISIBLE_SPACE);

		x = 0;
		for(; *p && x < ed->FullW; ++p, ++x)
		{
			screen_set(x, y, *p, color);
		}

		for(; x < ed->FullW; ++x)
		{
			screen_set(x, y, ' ', color);
		}
	}

	return y;
}

static u32 ed_render_goto_line(Editor *ed)
{
	const char *prompt = "Location: ";
	const char *s;
	u32 x, i, c, color;

	color = screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG);
	for(s = prompt, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, 0, c, color);
	}

	for(s = ed->Search, i = 0; x < ed->FullW; ++x, ++s, ++i)
	{
		screen_set(x, 0, (i < ed->SLen) ? *s : ' ',
			(i == ed->SCursor) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG));
	}

	return ed_render_dir(ed);
}

static void ed_render(Editor *ed)
{
	if(ed->CursorY < ed->PageY)
	{
		ed->PageY = ed->CursorY;
	}

	if(ed->CursorY >= ed->PageY + ed->PageH)
	{
		ed->PageY = ed->CursorY - ed->PageH + 1;
	}

	{
		u32 lines = ed_num_lines(ed);
		if(lines > ed->PageH && ed->PageY + ed->PageH >= lines)
		{
			ed->PageY = lines - ed->PageH;
		}
	}

	if(ed->ShowLineNr)
	{
		ed_render_linenr(ed);
	}
	else
	{
		ed->PageW = ed->FullW;
		ed->OffsetX = 0;
	}

	if(ed->CursorX < ed->PageX)
	{
		ed->PageX = ed->CursorX;
	}

	if(ed->CursorX >= ed->PageX + ed->PageW)
	{
		ed->PageX = ed->CursorX - ed->PageW + 1;
	}

	{
		u32 y, cursor_x, start_y, end_y;
		start_y = 0;
		end_y = ed->PageH;
		if(ed->Mode == EDITOR_MODE_GOTO)
		{
			start_y = ed_render_goto_line(ed);
		}
		else if(ed->Mode == EDITOR_MODE_MSG)
		{
			--end_y;
			ed_render_msg(ed);
		}

		cursor_x = cursor_pos_x(ed);
		for(y = start_y; y < end_y; ++y)
		{
			ed_render_line(ed, y, cursor_x);
		}
	}
}

static void ed_msg(Editor *ed, u32 msg_type, const char *msg, ...)
{
	va_list args;
	ed->Mode = EDITOR_MODE_MSG;
	ed->MsgType = msg_type;
	va_start(args, msg);
	vsnprintf(ed->Msg, MAX_MSG_LEN, msg, args);
	va_end(args);
	ed_render(ed);
}

static void ed_backspace(Editor *ed)
{
	Vector *line = ed_cur_line(ed);
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			/* merge with previous line */
			Vector *prev = ed_line_get(ed, --ed->CursorY);
			ed->CursorX = vector_len(prev);
			vector_push(prev, vector_len(line), vector_data(line));
			vector_destroy(line);
			ed_line_remove(ed, ed->CursorY + 1);
		}
	}
	else
	{
		/* delete prev char */
		vector_remove(line, --ed->CursorX, 1);
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_delete(Editor *ed)
{
	Vector *line = ed_cur_line(ed);
	u32 line_len = vector_len(line);
	if(ed->CursorX >= line_len)
	{
		u32 num_lines = ed_num_lines(ed);
		ed->CursorX = line_len;
		if(ed->CursorY < num_lines - 1)
		{
			/* merge with next line */
			u32 next_idx = ed->CursorY + 1;
			Vector *next = ed_line_get(ed, next_idx);
			vector_push(line, vector_len(next), vector_data(next));
			vector_destroy(next);
			ed_line_remove(ed, next_idx);
		}
	}
	else
	{
		/* delete next char */
		vector_remove(line, ed->CursorX, 1);
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_char(Editor *ed, u32 chr)
{
	u8 ins[1];
	ins[0] = chr;
	vector_insert(ed_cur_line(ed), ed->CursorX, 1, ins);
	++ed->CursorX;
	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_enter(Editor *ed)
{
	Vector new;
	Vector *cur = ed_cur_line(ed);
	char *str = (char *)vector_data(cur) + ed->CursorX;
	u32 len = vector_len(cur) - ed->CursorX;

	/* Copy characters after cursor on current line to new line */
	vector_init(&new, len);
	vector_push(&new, len, str);

	/* Insert new line */
	ed_line_insert(ed, ed->CursorY + 1, &new);

	/* Remove characters after cursor on current line */
	vector_remove(cur, ed->CursorX, len);

	++ed->CursorY;
	ed->CursorX = 0;
	ed->CursorSaveX = 0;
	ed_render(ed);
}

static void ed_home(Editor *ed)
{
	Vector *line = ed_cur_line(ed);
	char *buf = vector_data(line);
	u32 len = vector_len(line);
	u32 i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	ed->CursorX = (ed->CursorX == i) ? 0 : i;
	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_end(Editor *ed)
{
	ed->CursorX = ed_cur_line_len(ed);
	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_move_vertical(Editor *ed)
{
	u32 len = ed_cur_line_len(ed);
	ed->CursorX = ed->CursorSaveX;
	if(ed->CursorX > len)
	{
		ed->CursorX = len;
	}
}

static void ed_page_up(Editor *ed)
{
	if(ed->CursorY >= ed->PageH)
	{
		ed->CursorY -= ed->PageH;
	}
	else
	{
		ed->CursorY = 0;
		ed->CursorX = 0;
		ed->CursorSaveX = 0;
	}

	ed_move_vertical(ed);
	ed_render(ed);
}

static void ed_page_down(Editor *ed)
{
	u32 num_lines = ed_num_lines(ed);
	ed->CursorY += ed->PageH;
	if(ed->CursorY >= num_lines)
	{
		ed->CursorY = num_lines - 1;
		ed->CursorX = ed_cur_line_len(ed);
		ed->CursorSaveX = ed->CursorX;
	}

	ed_move_vertical(ed);
	ed_render(ed);
}

static void ed_left(Editor *ed)
{
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			--ed->CursorY;
			ed->CursorX = ed_cur_line_len(ed);
		}
	}
	else
	{
		--ed->CursorX;
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_right(Editor *ed)
{
	if(ed->CursorX == ed_cur_line_len(ed))
	{
		if(ed->CursorY < ed_num_lines(ed) - 1)
		{
			++ed->CursorY;
			ed->CursorX = 0;
		}
	}
	else
	{
		++ed->CursorX;
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_up(Editor *ed)
{
	if(ed->CursorY == 0)
	{
		ed->CursorX = 0;
		ed->CursorSaveX = 0;
	}
	else
	{
		--ed->CursorY;
		ed_move_vertical(ed);
	}

	ed_render(ed);
}

static void ed_down(Editor *ed)
{
	if(ed->CursorY >= ed_num_lines(ed) - 1)
	{
		ed->CursorX = ed_cur_line_len(ed);
		ed->CursorSaveX = ed->CursorX;
	}
	else
	{
		++ed->CursorY;
		ed_move_vertical(ed);
	}

	ed_render(ed);
}

static void ed_top(Editor *ed)
{
	ed->CursorX = 0;
	ed->CursorY = 0;
	ed->CursorSaveX = 0;
	ed_render(ed);
}

static void ed_bottom(Editor *ed)
{
	ed->CursorY = ed_num_lines(ed) - 1;
	ed->CursorX = ed_cur_line_len(ed);
	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_reset_cursor(Editor *ed)
{
	ed->PageY = 0;
	ed->PageX = 0;

	ed->CursorX = 0;
	ed->CursorSaveX = 0;
	ed->CursorY = 0;
}

static void ed_gotoxy(Editor *ed, u32 x, u32 y)
{
	u32 num_lines = ed_num_lines(ed);
	if(y >= num_lines)
	{
		y = num_lines - 1;
	}

	ed->CursorY = y;
	if(ed->CursorY > ed->PageH / 2)
	{
		ed->PageY = ed->CursorY - ed->PageH / 2;
	}

	ed->CursorSaveX = ed->CursorX = x;
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

static void ed_goto_def(Editor *ed, const char *s)
{
	u32 i;
	u32 len = ed_num_lines(ed);
	u32 sl = strlen(s);
	for(i = 0; i < len; ++i)
	{
		Vector *line = ed_line_get(ed, i);
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

		ed_gotoxy(ed, offset, i);
		return;
	}
}

static void ed_init(Editor *ed, u32 width, u32 height)
{
	Vector line;
	vector_init(&ed->Lines, 128 * sizeof(Vector));
	vector_init(&line, 8);
	vector_push(&ed->Lines, sizeof(line), &line);

	ed_reset_cursor(ed);

	ed->TabSize = 4;
	ed->ShowWhitespace = 1;

	ed->FullW = width;
	ed->PageH = height;

	ed->ShowLineNr = 1;

	strcpy(ed->SBuf, "./");
	ed->Search = ed->SBuf + 2;
}

static void ed_sel_store(Editor *ed)
{
	(void)ed;
}

static void ed_sel_delete(Editor *ed)
{
	(void)ed;
}

static void ed_cut(Editor *ed)
{
	ed_sel_store(ed);
	ed_sel_delete(ed);
	ed_render(ed);
}

static void ed_copy(Editor *ed)
{
	ed_sel_store(ed);
}

static void ed_paste(Editor *ed)
{
	/* ed_insert(ed, clipboard_load()); */
	ed_render(ed);
}

static void ed_prev_word(Editor *ed)
{
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			--ed->CursorY;
			ed->CursorX = ed_cur_line_len(ed);
		}
	}
	else
	{
		if(ed->CursorX > 0)
		{
			char *buf = vector_data(ed_cur_line(ed));
			u32 i = ed->CursorX - 1;
			u32 type = char_type(buf[i]);
			while(i > 0 && char_type(buf[i - 1]) == type) { --i; }
			ed->CursorX = i;
		}
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_next_word(Editor *ed)
{
	u32 len = ed_cur_line_len(ed);
	if(ed->CursorX == len)
	{
		if(ed->CursorY < ed_num_lines(ed) - 1)
		{
			++ed->CursorY;
			ed->CursorX = 0;
		}
	}
	else
	{
		if(ed->CursorX < len)
		{
			char *buf = vector_data(ed_cur_line(ed));
			u32 i = ed->CursorX;
			u32 type = char_type(buf[i]);
			while(i < len && char_type(buf[i]) == type) { ++i; }
			ed->CursorX = i;
		}
	}

	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_move_up(Editor *ed)
{
	if(ed->PageY > 0)
	{
		--ed->PageY;
	}

	ed_render(ed);
}

static void ed_move_down(Editor *ed)
{
	++ed->PageY;
	ed_render(ed);
}

static void ed_toggle_line_nr(Editor *ed)
{
	ed->ShowLineNr = !ed->ShowLineNr;
	ed_render(ed);
}

static void ed_tab_size(Editor *ed)
{
	ed->TabSize <<= 1;
	if(ed->TabSize > 8)
	{
		ed->TabSize = 2;
	}

	ed_render(ed);
}

static void ed_whitespace(Editor *ed)
{
	ed->ShowWhitespace = !ed->ShowWhitespace;
	ed_render(ed);
}

static void ed_clear(Editor *ed)
{
	u32 i;
	u32 num_lines = ed_num_lines(ed);
	for(i = 0; i < num_lines; ++i)
	{
		vector_destroy(ed_line_get(ed, i));
	}

	vector_clear(&ed->Lines);
}

static void ed_load_text(Editor *ed, const char *buf)
{
	u32 c;
	Vector line;
	const char *linestart, *p;
	ed_clear(ed);
	ed_reset_cursor(ed);
	p = buf;
	linestart = buf;
	do
	{
		c = *p;
		if(c == '\0' || c == '\n')
		{
			vector_from(&line, linestart, p - linestart);
			vector_push(&ed->Lines, sizeof(line), &line);
			linestart = p + 1;
		}
		++p;
	}
	while(c != '\0');
}

static void ed_load(Editor *ed, const char *filename)
{
	char *buf;
	u32 status = textfile_read(filename, &buf);
	switch(status)
	{
	case FILE_READ_FAIL:
		ed_msg(ed, EDITOR_ERROR, "Failed to open file");
		return;

	case FILE_READ_NOT_TEXT:
		ed_msg(ed, EDITOR_ERROR, "Invalid character, binary file?");
		return;
	}

	ed->Mode = EDITOR_MODE_DEFAULT;
	strcpy(ed->FileName, filename);
	ed_load_text(ed, buf);
	_free(buf);
}

static u32 ed_count_bytes(Editor *ed)
{
	u32 i, len;
	u32 num_lines = ed_num_lines(ed);
	for(i = 0, len = 0; i < num_lines; ++i)
	{
		len += ed_line_len(ed, i) + 1;
	}

	return len - 1;
}

static void ed_save(Editor *ed)
{
	u32 i;
	u32 len = ed_count_bytes(ed);
	char *buf = _malloc(len);
	char *p = buf;
	u32 num_lines = ed_num_lines(ed);
	for(i = 0; i < num_lines; ++i)
	{
		Vector *cur = ed_line_get(ed, i);
		u32 line_len = vector_len(cur);
		memcpy(p, vector_data(cur), line_len);
		p += line_len;
		if(i < num_lines - 1)
		{
			*p++ = '\n';
		}
	}

	if(file_write(ed->FileName, buf, len))
	{
		ed_msg(ed, EDITOR_ERROR, "Writing file failed");
	}
	else
	{
		ed_msg(ed, EDITOR_INFO, "File saved");
	}

	_free(buf);
}

static void ed_inc(Editor *ed, const char *s)
{
	vector_insert(ed_cur_line(ed), ed->CursorX, 11, s);
	ed->CursorX += 10;
	ed->CursorSaveX = ed->CursorX;
	ed_render(ed);
}

static void ed_include(Editor *ed)
{
	static const char *ins = "#include \"\"";
	ed_inc(ed, ins);
}

static void ed_include_lib(Editor *ed)
{
	static const char *ins = "#include <>";
	ed_inc(ed, ins);
}

#include "nav.c"

static void ed_key_press_default(Editor *ed, u32 key, u32 cp)
{
	static EditorKeyBind key_events[] =
	{
		{ MOD_CTRL | KEY_LEFT,  ed_prev_word      },
		{ KEY_LEFT,             ed_left           },
		{ MOD_CTRL | KEY_RIGHT, ed_next_word      },
		{ KEY_RIGHT,            ed_right          },
		{ MOD_CTRL | KEY_UP,    ed_move_up        },
		{ KEY_UP,               ed_up             },
		{ MOD_CTRL | KEY_DOWN,  ed_move_down      },
		{ KEY_DOWN,             ed_down           },
		{ KEY_PAGE_UP,          ed_page_up        },
		{ KEY_PAGE_DOWN,        ed_page_down      },

		{ MOD_CTRL | KEY_HOME,  ed_top            },
		{ KEY_HOME,             ed_home           },
		{ MOD_CTRL | KEY_END,   ed_bottom         },
		{ KEY_END,              ed_end            },

		{ KEY_RETURN,           ed_enter          },
		{ KEY_BACKSPACE,        ed_backspace      },
		{ KEY_DELETE,           ed_delete         },

		{ MOD_CTRL | KEY_L,     ed_toggle_line_nr },
		{ MOD_CTRL | KEY_G,     ed_goto           },
		{ MOD_CTRL | KEY_O,     ed_open           },
		{ MOD_CTRL | KEY_C,     ed_copy           },
		{ MOD_CTRL | KEY_X,     ed_cut            },
		{ MOD_CTRL | KEY_V,     ed_paste          },
		{ MOD_CTRL | KEY_S,     ed_save           },

		{ MOD_CTRL | KEY_T,     ed_tab_size       },
		{ MOD_CTRL | KEY_J,     ed_whitespace     },

		{ MOD_CTRL | KEY_I,             ed_include     },
		{ MOD_CTRL | MOD_SHIFT | KEY_I, ed_include_lib },
	};

	u32 i;
	for(i = 0; i < ARRLEN(key_events); ++i)
	{
		EditorKeyBind *kb = key_events + i;
		if(kb->Key == key)
		{
			kb->Event(ed);
			return;
		}
	}

	if(isprint(cp) || cp == '\t')
	{
		ed_char(ed, cp);
	}
}

static void ed_key_press_msg(Editor *ed, u32 key, u32 cp)
{
	if((ed->MsgType == EDITOR_ERROR && key == KEY_RETURN) ||
		(ed->MsgType == EDITOR_INFO))
	{
		ed->Mode = EDITOR_MODE_DEFAULT;
		ed_render(ed);
	}

	(void)cp;
}

static void ed_key_press(Editor *ed, u32 key, u32 cp)
{
	typedef void (*KeyPressFN)(Editor *, u32, u32);
	static const KeyPressFN fns[EDITOR_MODE_COUNT] =
	{
		ed_key_press_default,
		ed_key_press_nav,
		ed_key_press_msg
	};

	fns[ed->Mode](ed, key, cp);
}

static Editor editor;

static void event_keyboard(u32 key, u32 cp, u32 state)
{
	if(state == KEYSTATE_RELEASED)
	{
		return;
	}

	ed_key_press(&editor, key, cp);
}

static void event_resize(u32 w, u32 h)
{
	editor.FullW = w;
	editor.PageH = h;
	ed_render(&editor);
}

static void event_init(int argc, char *argv[], u32 w, u32 h)
{
#ifndef NDEBUG
	test_run_all();
#endif

	keyword_init();
	ed_init(&editor, w, h);

	if(argc == 2)
	{
		ed_load(&editor, argv[1]);
	}

	ed_render(&editor);
}

static u32 event_exit(void)
{
	ed_clear(&editor);
	vector_destroy(&editor.Lines);
	return 1;
}
