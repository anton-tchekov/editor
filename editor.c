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
#define ED_DIR_PAGE        12

typedef struct
{
	u32 X, Y;
} Cursor;

typedef struct
{
	Cursor Limits[2];
} Selection;

typedef struct
{
	Vector Lines;
	u32 CursorX;
	i32 CursorSaveX;
	u32 CursorY;
	u32 PageW;
	u32 FullW;
	u32 OffsetX;
	u32 PageH;
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

	char **DirList;
	u32 DirEntries, DirOffset, DirPos;
} Editor;

typedef void (*EditorFunc)(Editor *);

typedef struct
{
	u16 Key;
	EditorFunc Event;
} EditorKeyBind;

static void cursor_swap(Cursor *a, Cursor *b)
{
	Cursor temp = *a;
	*a = *b;
	*b = temp;
}

static u32 cursor_unordered(Cursor *a, Cursor *b)
{
	return (b->Y < a->Y) || (a->Y == b->Y && b->X < a->X);
}

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

static const char *ed_line_data(Editor *ed, u32 i)
{
	return vector_data(ed_line_get(ed, i));
}

static u32 ed_cur_line_len(Editor *ed)
{
	return vector_len(ed_cur_line(ed));
}

static void ed_cursor_norm(Editor *ed, Cursor *cursor)
{
	u32 len = ed_line_len(ed, cursor->Y);
	cursor->X = (cursor->X < len) ? cursor->X : len;
}

static void ed_sel_norm(Editor *ed, Selection *sel)
{
	Cursor *first = &sel->Limits[0];
	Cursor *second = &sel->Limits[1];

	ed_cursor_norm(ed, first);
	ed_cursor_norm(ed, second);

	if(cursor_unordered(first, second))
	{
		cursor_swap(first, second);
	}
}

static u32 ed_sel_count_bytes(Editor *ed, const Selection *sel)
{
	u32 len;
	u32 x1 = sel->Limits[0].X;
	u32 y1 = sel->Limits[0].Y;
	u32 x2 = sel->Limits[1].X;
	u32 y2 = sel->Limits[1].Y;
	if(y1 == y2)
	{
		return x2 - x1;
	}

	len = ed_line_len(ed, y1) - x1 + 1;
	for(++y1; y1 < y2; ++y1)
	{
		len += ed_line_len(ed, y1) + 1;
	}

	return len + x2;
}

static char *ed_sel_get(Editor *ed, const Selection *sel, u32 *out_len)
{
	u32 x1, y1, x2, y2, len;
	char *output, *p;
	Selection nsel = *sel;
	ed_sel_norm(ed, &nsel);

	x1 = nsel.Limits[0].X;
	y1 = nsel.Limits[0].Y;
	x2 = nsel.Limits[1].X;
	y2 = nsel.Limits[1].Y;

	*out_len = len = ed_sel_count_bytes(ed, &nsel);
	p = output = _malloc(len + 1);
	if(y1 == y2)
	{
		memcpy(p, ed_line_data(ed, y1) + x1, len);
		p += len;
	}
	else
	{
		Vector *line = ed_line_get(ed, y1);
		len = vector_len(line) - x1;
		memcpy(p, (u8 *)vector_data(line) + x1, len);
		p += len;
		for(++y1; y1 < y2; ++y1)
		{
			*p++ = '\n';
			line = ed_line_get(ed, y1);
			len = vector_len(line);
			memcpy(p, vector_data(line), len);
			p += len;
		}

		*p++ = '\n';
		memcpy(p, ed_line_data(ed, y1), x2);
		p += x2;
	}

	*p = '\0';
	return output;
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
			char lnr_buf[16];
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

static void ed_put(Editor *ed, u32 x, u32 y, u32 cursor_x, u32 c)
{
	if(y == ed->CursorY && x == cursor_x)
	{
		c = screen_pack_color_swap(c);
	}

	screen_set_pack(x + ed->OffsetX, y - ed->PageY, c);
}

static u32 ed_syntax_sub(Editor *ed,
	u32 c, u32 color, u32 y, u32 x, u32 cursor_x)
{
	if(c == '\t')
	{
		u32 n = x & (ed->TabSize - 1);
		if(ed->ShowWhitespace)
		{
			color = screen_color(COLOR_TABLE_VISIBLE_SPACE, COLOR_TABLE_BG);
			if(n == ed->TabSize - 1)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x,
					screen_pack(CHAR_TAB_BOTH, color));
			}
			else
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x,
					screen_pack(CHAR_TAB_START, color));
				for(++n; n < ed->TabSize - 1; ++n)
				{
					if(x >= ed->PageW) { return x; }
					ed_put(ed, x++, y, cursor_x,
						screen_pack(CHAR_TAB_MIDDLE, color));
				}
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x,
					screen_pack(CHAR_TAB_END, color));
			}
		}
		else
		{
			for(; n < ed->TabSize; ++n)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x, screen_pack(' ', color));
			}
		}
	}
	else
	{
		if(c == ' ' && ed->ShowWhitespace)
		{
			c = CHAR_VISIBLE_SPACE;
			color = COLOR_TABLE_VISIBLE_SPACE;
		}

		if(x >= ed->PageW) { return x; }
		ed_put(ed, x++, y, cursor_x, screen_pack(c,
			screen_color(color, COLOR_TABLE_BG)));
	}

	return x;
}

static u32 ed_syntax(Editor *ed, u32 y, u32 cursor_x)
{
	Vector *lv = ed_line_get(ed, y);
	u32 len = vector_len(lv);
	const char *line = vector_data(lv);
	u32 incflag = 0;
	u32 i = 0;
	u32 x = 0;
	while(i < len)
	{
		u32 c = line[i];
		if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(ed, line[i], COLOR_TABLE_COMMENT, y, x, cursor_x);
				if(x >= ed->PageW) { return x; }
			}
		}
		else if(c == '#')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, cursor_x, screen_pack(c,
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x, screen_pack(c,
					screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save = c == '<' ? '>' : c;
			u32 esc = 0;
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, cursor_x, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(ed, c, COLOR_TABLE_STRING, y, x, cursor_x);
				if(x >= ed->PageW) { return x; }

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
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, cursor_x, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '[' || c == ']')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, cursor_x, screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '{' || c == '}')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, cursor_x, screen_pack(c,
				screen_color(COLOR_TABLE_BRACE, COLOR_TABLE_BG)));
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
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, cursor_x, screen_pack(line[i], color));
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
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x, y, cursor_x, screen_pack(line[i],
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(ed, c, COLOR_TABLE_FG, y, x, cursor_x);
			if(x >= ed->PageW) { return x; }
			++i;
		}
	}

	return x;
}

static void ed_render_line(Editor *ed, u32 y, u32 cursor_x)
{
	u32 x = 0;
	u32 line = ed->PageY + y;
	if(line < ed_num_lines(ed))
	{
		x = ed_syntax(ed, line, cursor_x);
	}

	for(; x < ed->PageW; ++x)
	{
		ed_put(ed, x, line, cursor_x, screen_pack(' ',
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
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
	u32 i;
	u32 y = 1;
	u32 end = ed->DirOffset + ED_DIR_PAGE;
	if(end > ed->DirEntries)
	{
		end = ed->DirEntries;
	}

	for(i = ed->DirOffset; i < end; ++i, ++y)
	{
		u32 color = (i == ed->DirPos) ?
			screen_color(COLOR_TABLE_INFO, COLOR_TABLE_VISIBLE_SPACE) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_VISIBLE_SPACE);

		u32 x = 0;
		const char *p = ed->DirList[i];
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

static u32 ed_chr_x_inc(Editor *ed, u32 c, u32 x)
{
	return (c == '\t') ?
		((x + ed->TabSize) / ed->TabSize * ed->TabSize) :
		(x + 1);
}

static u32 ed_cursor_pos_x(Editor *ed, u32 y)
{
	u32 i;
	u32 x = 0;
	const char *line = vector_data(ed_line_get(ed, y));
	for(i = 0; i < ed->CursorX; ++i)
	{
		x = ed_chr_x_inc(ed, line[i], x);
	}

	return x;
}

static void ed_render(Editor *ed)
{
	u32 y;
	u32 cursor_x;
	u32 start_y = 0;
	u32 end_y = ed->PageH;
	u32 lines = ed_num_lines(ed);

	if(ed->CursorY < ed->PageY)
	{
		ed->PageY = ed->CursorY;
	}

	if(ed->CursorY >= ed->PageY + ed->PageH)
	{
		ed->PageY = ed->CursorY - ed->PageH + 1;
	}

	if(lines > ed->PageH && ed->PageY + ed->PageH >= lines)
	{
		ed->PageY = lines - ed->PageH;
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

	if(ed->Mode == EDITOR_MODE_GOTO)
	{
		start_y = ed_render_goto_line(ed);
	}
	else if(ed->Mode == EDITOR_MODE_MSG)
	{
		--end_y;
		ed_render_msg(ed);
	}

	cursor_x = ed_cursor_pos_x(ed, ed->CursorY);
	for(y = start_y; y < end_y; ++y)
	{
		ed_render_line(ed, y, cursor_x);
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

	ed->CursorSaveX = -1;
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

	ed->CursorSaveX = -1;
	ed_render(ed);
}

static void ed_char(Editor *ed, u32 chr)
{
	u8 ins[1];
	ins[0] = chr;
	vector_insert(ed_cur_line(ed), ed->CursorX, 1, ins);
	++ed->CursorX;
	ed->CursorSaveX = -1;
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
	ed->CursorSaveX = -1;
	ed_render(ed);
}

static void ed_end(Editor *ed)
{
	ed->CursorX = ed_cur_line_len(ed);
	ed->CursorSaveX = -1;
	ed_render(ed);
}

static void ed_move_vertical(Editor *ed, u32 prev_y)
{
	Vector *line = ed_cur_line(ed);
	u32 len = vector_len(line);
	const char *buf = vector_data(line);
	u32 i, x, max_x;

	if(ed->CursorSaveX < 0)
	{
		ed->CursorSaveX = ed_cursor_pos_x(ed, prev_y);
	}

	max_x = ed->CursorSaveX;
	for(i = 0, x = 0; i < len && x < max_x; ++i)
	{
		x = ed_chr_x_inc(ed, buf[i], x);
	}

	ed->CursorX = i;
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
		u32 prev_y = ed->CursorY;
		--ed->CursorY;
		ed_move_vertical(ed, prev_y);
	}

	ed_render(ed);
}

static void ed_down(Editor *ed)
{
	if(ed->CursorY >= ed_num_lines(ed) - 1)
	{
		ed->CursorX = ed_cur_line_len(ed);
		ed->CursorSaveX = -1;
	}
	else
	{
		u32 prev_y = ed->CursorY;
		++ed->CursorY;
		ed_move_vertical(ed, prev_y);
	}

	ed_render(ed);
}

static void ed_page_up(Editor *ed)
{
	if(ed->CursorY >= ed->PageH)
	{
		u32 prev_y = ed->CursorY;
		ed->CursorY -= ed->PageH;
		ed_move_vertical(ed, prev_y);
	}
	else
	{
		ed->CursorY = 0;
		ed->CursorX = 0;
		ed->CursorSaveX = 0;
	}

	ed_render(ed);
}

static void ed_page_down(Editor *ed)
{
	u32 num_lines = ed_num_lines(ed);
	u32 prev_y = ed->CursorY;
	ed->CursorY += ed->PageH;
	if(ed->CursorY >= num_lines)
	{
		ed->CursorY = num_lines - 1;
		ed->CursorX = ed_cur_line_len(ed);
		ed->CursorSaveX = -1;
	}
	else
	{
		ed_move_vertical(ed, prev_y);
	}

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

	ed->CursorSaveX = -1;
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

	ed->CursorSaveX = -1;
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
	ed->CursorSaveX = -1;
	ed_render(ed);
}

static void ed_reset_cursor(Editor *ed)
{
	ed->PageY = 0;
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

	ed->CursorX = x;
	ed->CursorSaveX = -1;
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

	ed->CursorSaveX = -1;
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

	ed->CursorSaveX = -1;
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
	ed->CursorSaveX = -1;
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
	_free(editor.DirList);
	return 1;
}
