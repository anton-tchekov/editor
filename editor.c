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

enum
{
	LANGUAGE_UNKNOWN,
	LANGUAGE_C
};

#define MAX_SEARCH_LEN    256
#define MAX_MSG_LEN        80
#define ED_DIR_PAGE        12
#define COMMENT_LOOKBACK   10

typedef struct
{
	u32 X, Y;
} Cursor;

typedef struct
{
	Cursor C[2];
} Selection;

typedef struct
{
	Vector Lines;

	Selection Sel, VSel;
	Cursor VCursor;

	i32 CursorSaveX;
	u32 FullW, OffsetX, PageW, PageH, PageY;

	u8 InComment;

	u8 TabSize;
	u8 ShowLineNr;
	u8 ShowWhitespace;
	u8 Mode;
	u8 Language;

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

static void ed_sel_to_cursor(Editor *ed)
{
	ed->Sel.C[0] = ed->Sel.C[1];
}

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

static u32 sel_wide(Selection *sel)
{
	return !(sel->C[0].X == sel->C[1].X && sel->C[0].Y == sel->C[1].Y);
}

static void sel_left(Selection *sel)
{
	if(cursor_unordered(&sel->C[0], &sel->C[1]))
	{
		sel->C[0] = sel->C[1];
	}
	else
	{
		sel->C[1] = sel->C[0];
	}
}

static void sel_right(Selection *sel)
{
	if(cursor_unordered(&sel->C[0], &sel->C[1]))
	{
		sel->C[1] = sel->C[0];
	}
	else
	{
		sel->C[0] = sel->C[1];
	}
}

static void ed_sel_norm(Selection *sel)
{
	if(cursor_unordered(&sel->C[0], &sel->C[1]))
	{
		cursor_swap(&sel->C[0], &sel->C[1]);
	}
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
	return ed_line_get(ed, ed->Sel.C[1].Y);
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

static u32 ed_sel_count_bytes(Editor *ed, Selection *sel)
{
	u32 len;
	u32 x1 = sel->C[0].X;
	u32 y1 = sel->C[0].Y;
	u32 x2 = sel->C[1].X;
	u32 y2 = sel->C[1].Y;
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

static char *ed_sel_get(Editor *ed, Selection *sel, u32 *out_len)
{
	u32 x1, y1, x2, y2, len;
	char *output, *p;
	Selection nsel = *sel;
	ed_sel_norm(&nsel);

	x1 = nsel.C[0].X;
	y1 = nsel.C[0].Y;
	x2 = nsel.C[1].X;
	y2 = nsel.C[1].Y;

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

static void ed_remove_lines(Editor *ed, u32 y1, u32 y2)
{
	u32 i;
	for(i = y1; i <= y2; ++i)
	{
		vector_destroy(ed_line_get(ed, i));
	}

	vector_remove(&ed->Lines, y1 * sizeof(Vector),
		(y2 - y1 + 1) * sizeof(Vector));
}

static void ed_sel_delete(Editor *ed)
{
	u32 x1, y1, x2, y2;
	Vector *line;
	Selection nsel = ed->Sel;
	ed_sel_norm(&nsel);

	x1 = nsel.C[0].X;
	y1 = nsel.C[0].Y;
	x2 = nsel.C[1].X;
	y2 = nsel.C[1].Y;

	ed->Sel.C[1] = ed->Sel.C[0] = nsel.C[0];

	line = ed_line_get(ed, y1);
	if(y1 == y2)
	{
		vector_remove(line, x1, x2 - x1);
	}
	else
	{
		Vector *last = ed_line_get(ed, y2);
		vector_replace(line, x1, vector_len(line) - x1,
			(u8 *)vector_data(last) + x2, vector_len(last) - x2);
		ed_remove_lines(ed, y1 + 1, y2);
	}
}

static void ed_sel_clear(Editor *ed)
{
	if(sel_wide(&ed->Sel))
	{
		ed_sel_delete(ed);
	}
}

static void ed_ins_lines(Editor *ed, u32 y, u32 count)
{
	vector_makespace(&ed->Lines, y * sizeof(Vector), count * sizeof(Vector));
}

static void ed_insert(Editor *ed, const char *text)
{
	u32 c, y, new_lines;
	const char *p, *s;

	ed_sel_clear(ed);

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

	y = ed->Sel.C[1].Y;
	if(!new_lines)
	{
		u32 len = s - text;
		vector_insert(ed_line_get(ed, y), ed->Sel.C[1].X, len, text);
		ed->Sel.C[1].X += len;
	}
	else
	{
		char *last;
		u32 slen, rlen;
		Vector line, *first, *lines;

		/* Insert new lines in advance to avoid quadratic complexity */
		ed_ins_lines(ed, y + 1, new_lines);
		lines = vector_data(&ed->Lines);

		/* Last line */
		first = ed_line_get(ed, y);
		rlen = vector_len(first) - ed->Sel.C[1].X;
		slen = s - p;
		vector_init(&line, rlen + slen);
		line.Length = rlen + slen;
		last = vector_data(&line);
		memcpy(last, p, slen);
		memcpy(last + slen, (u8 *)vector_data(first) + ed->Sel.C[1].X, rlen);
		lines[y + new_lines] = line;

		first->Length = ed->Sel.C[1].X;
		ed->Sel.C[1].X = slen;
		ed->Sel.C[1].Y = y + new_lines;

		/* First line */
		p = strchr(text, '\n');
		vector_push(ed_line_get(ed, y), p - text, text);
		text = p + 1;

		/* Other lines */
		while((p = strchr(text, '\n')))
		{
			vector_from(&line, text, p - text);
			lines[++y] = line;
			text = p + 1;
		}
	}

	ed->CursorSaveX = -1;
	ed_sel_to_cursor(ed);
}

static void ed_line_remove(Editor *ed, u32 line)
{
	vector_destroy(ed_line_get(ed, line));
	vector_remove(&ed->Lines, line * sizeof(Vector), sizeof(Vector));
}

static void ed_line_insert(Editor *ed, u32 line, Vector *v)
{
	vector_insert(&ed->Lines, line * sizeof(Vector), sizeof(Vector), v);
}

static void ed_render_linenr(Editor *ed)
{
	u32 x, y, lnr_max, lnr_width;

	u32 def = screen_color(COLOR_TABLE_GRAY, COLOR_TABLE_BG);
	u32 cur = screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG);

	u32 lines = ed_num_lines(ed);
	u32 lnr = ed->PageY;
	lnr_max = lnr + ed->PageH;
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);

	for(y = 0; y < ed->PageH; ++y)
	{
		u32 color = (lnr == ed->Sel.C[1].Y) ? cur : def;
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

	ed->PageW = ed->FullW - lnr_width - 1;
	ed->OffsetX = lnr_width + 1;
}

static u32 is_sel(Editor *ed, u32 x, u32 y)
{
	if(y < ed->VSel.C[0].Y || y > ed->VSel.C[1].Y)
	{
		return 0;
	}

	if(y == ed->VSel.C[0].Y && x < ed->VSel.C[0].X)
	{
		return 0;
	}

	if(y == ed->VSel.C[1].Y && x >= ed->VSel.C[1].X)
	{
		return 0;
	}

	return 1;
}

static u32 is_cursor(Editor *ed, u32 x, u32 y)
{
	return y == ed->VCursor.Y && x == ed->VCursor.X;
}

static void ed_put(Editor *ed, u32 x, u32 y, u32 c)
{
	if(is_cursor(ed, x, y))
	{
		c = screen_pack_color_swap(c);
	}
	else if(is_sel(ed, x, y))
	{
		c = screen_pack_set_bg(c, COLOR_TABLE_SELECTION);
	}

	screen_set(x + ed->OffsetX, y - ed->PageY, c);
}

static u32 ed_syntax_sub(Editor *ed, u32 c, u32 color, u32 y, u32 x)
{
	if(c == '\t')
	{
		u32 tabsize = ed->TabSize;
		u32 n = x & (tabsize - 1);
		if(ed->ShowWhitespace)
		{
			color = screen_color(COLOR_TABLE_GRAY, COLOR_TABLE_BG);
			if(n == tabsize - 1)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, screen_pack(CHAR_TAB_BOTH, color));
			}
			else
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, screen_pack(CHAR_TAB_START, color));
				for(++n; n < tabsize - 1; ++n)
				{
					if(x >= ed->PageW) { return x; }
					ed_put(ed, x++, y, screen_pack(CHAR_TAB_MIDDLE, color));
				}
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, screen_pack(CHAR_TAB_END, color));
			}
		}
		else
		{
			for(; n < tabsize; ++n)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, screen_pack(' ', color));
			}
		}
	}
	else
	{
		if(c == ' ' && ed->ShowWhitespace)
		{
			c = CHAR_VISIBLE_SPACE;
			color = COLOR_TABLE_GRAY;
		}

		if(x >= ed->PageW) { return x; }
		ed_put(ed, x++, y, screen_pack(c, screen_color(color, COLOR_TABLE_BG)));
	}

	return x;
}

static u32 ed_plain(Editor *ed, u32 y)
{
	Vector *lv = ed_line_get(ed, y);
	const char *line = vector_data(lv);
	u32 len = vector_len(lv);
	u32 i, x;
	for(x = 0, i = 0; i < len; ++i)
	{
		x = ed_syntax_sub(ed, line[i], COLOR_TABLE_FG, y, x);
		if(x >= ed->PageW) { return x; }
	}

	return x;
}

static u32 ed_syntax(Editor *ed, u32 y)
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
		if(ed->InComment)
		{
			x = ed_syntax_sub(ed, c, COLOR_TABLE_COMMENT, y, x);
			if(x >= ed->PageW) { return x; }

			if((c == '*') && (i + 1 < len) && (line[i + 1] == '/'))
			{
				++i;
				ed->InComment = 0;
				ed_put(ed, x++, y, screen_pack('/',
					screen_color(COLOR_TABLE_COMMENT, COLOR_TABLE_BG)));
				if(x >= ed->PageW) { return x; }
			}
			++i;
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '/'))
		{
			for(; i < len; ++i)
			{
				x = ed_syntax_sub(ed, line[i], COLOR_TABLE_COMMENT, y, x);
				if(x >= ed->PageW) { return x; }
			}
		}
		else if((c == '/') && (i + 1 < len) && (line[i + 1] == '*'))
		{
			ed->InComment = 1;
		}
		else if(c == '#')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			for(++i; i < len && isalnum(c = line[i]); ++i)
			{
				if(x >= ed->PageW) { return x; }
				ed_put(ed, x++, y, screen_pack(c,
					screen_color(COLOR_TABLE_KEYWORD, COLOR_TABLE_BG)));
			}
			incflag = 1;
		}
		else if(c == '\"' || c == '\'' || (c == '<' && incflag))
		{
			u32 save = c == '<' ? '>' : c;
			u32 esc = 0;
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_STRING, COLOR_TABLE_BG)));
			for(++i; i < len; ++i)
			{
				c = line[i];
				x = ed_syntax_sub(ed, c, COLOR_TABLE_STRING, y, x);
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
			ed_put(ed, x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_PAREN, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '[' || c == ']')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, screen_pack(c,
				screen_color(COLOR_TABLE_BRACKET, COLOR_TABLE_BG)));
			++i;
		}
		else if(c == '{' || c == '}')
		{
			if(x >= ed->PageW) { return x; }
			ed_put(ed, x++, y, screen_pack(c,
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
				ed_put(ed, x++, y, screen_pack(line[i], color));
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
				ed_put(ed, x, y, screen_pack(line[i],
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG)));
			}
		}
		else
		{
			x = ed_syntax_sub(ed, c, COLOR_TABLE_FG, y, x);
			if(x >= ed->PageW) { return x; }
			++i;
		}
	}

	return x;
}

static void ed_render_line(Editor *ed, u32 y)
{
	u32 x = 0;
	u32 line = ed->PageY + y;
	if(line < ed_num_lines(ed))
	{
		switch(ed->Language)
		{
		case LANGUAGE_UNKNOWN:
			x = ed_plain(ed, line);
			break;

		case LANGUAGE_C:
			x = ed_syntax(ed, line);
			break;
		}
	}

	if(x < ed->PageW)
	{
		ed_put(ed, x++, line, screen_pack(' ',
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
	}

	for(; x < ed->PageW; ++x)
	{
		screen_set(x + ed->OffsetX, y,
			screen_pack(' ', screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG)));
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
		screen_set(x, y, screen_pack(c, color));
	}

	for(; x < ed->FullW; ++x)
	{
		screen_set(x, y, screen_pack(' ', color));
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
			screen_color(COLOR_TABLE_INFO, COLOR_TABLE_GRAY) :
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_GRAY);

		u32 x = 0;
		const char *p = ed->DirList[i];
		for(; *p && x < ed->FullW; ++p, ++x)
		{
			screen_set(x, y, screen_pack(*p, color));
		}

		for(; x < ed->FullW; ++x)
		{
			screen_set(x, y, screen_pack(' ', color));
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
		screen_set(x, 0, screen_pack(c, color));
	}

	for(s = ed->Search, i = 0; x < ed->FullW; ++x, ++s, ++i)
	{
		screen_set(x, 0, screen_pack((i < ed->SLen) ? *s : ' ',
			(i == ed->SCursor) ?
			screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG) :
			screen_color(COLOR_TABLE_BG, COLOR_TABLE_FG)));
	}

	return ed_render_dir(ed);
}

static u32 ed_chr_x_inc(Editor *ed, u32 c, u32 x)
{
	return (c == '\t') ?
		((x + ed->TabSize) / ed->TabSize * ed->TabSize) :
		(x + 1);
}

static u32 ed_cursor_pos_x(Editor *ed, u32 y, u32 end)
{
	u32 i;
	u32 x = 0;
	const char *line = vector_data(ed_line_get(ed, y));
	for(i = 0; i < end; ++i)
	{
		x = ed_chr_x_inc(ed, line[i], x);
	}

	return x;
}

static u32 ed_prev_comment(Editor *ed)
{
	u32 i = 0, in_comment = 0;
	if(ed->PageY > COMMENT_LOOKBACK)
	{
		i = ed->PageY - COMMENT_LOOKBACK;
	}

	for(; i < ed->PageY; ++i)
	{
		i32 p;
		Vector *line = ed_line_get(ed, i);
		const char *data = vector_data(line);
		i32 len = vector_len(line);
		for(p = 0; p < len - 1; ++p)
		{
			if(in_comment)
			{
				if(data[p] == '*' && data[p + 1] == '/')
				{
					++p;
					in_comment = 0;
				}
			}
			else
			{
				if(data[p] == '/' && data[p + 1] == '*')
				{
					++p;
					in_comment = 1;
				}
			}
		}
	}

	return in_comment;
}

static void ed_render(Editor *ed)
{
	u32 y;
	u32 start_y = 0;
	u32 end_y = ed->PageH;
	u32 lines = ed_num_lines(ed);

	if(ed->Sel.C[1].Y < ed->PageY)
	{
		ed->PageY = ed->Sel.C[1].Y;
	}

	if(ed->Sel.C[1].Y >= ed->PageY + ed->PageH)
	{
		ed->PageY = ed->Sel.C[1].Y - ed->PageH + 1;
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

	ed->VCursor.X = ed_cursor_pos_x(ed, ed->Sel.C[1].Y, ed->Sel.C[1].X);
	ed->VCursor.Y = ed->Sel.C[1].Y;
	ed->VSel.C[0] = ed->VCursor;
	if(ed->Sel.C[0].Y == ed->Sel.C[1].Y && ed->Sel.C[1].X == ed->Sel.C[0].X)
	{
		ed->VSel.C[1] = ed->VCursor;
	}
	else
	{
		ed->VSel.C[1].X = ed_cursor_pos_x(ed, ed->Sel.C[0].Y, ed->Sel.C[0].X);
		ed->VSel.C[1].Y = ed->Sel.C[0].Y;
	}

	if(ed->Language != LANGUAGE_UNKNOWN)
	{
		ed->InComment = ed_prev_comment(ed);
	}

	ed_sel_norm(&ed->VSel);
	for(y = start_y; y < end_y; ++y)
	{
		ed_render_line(ed, y);
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
	if(sel_wide(&ed->Sel))
	{
		ed_sel_delete(ed);
	}
	else
	{
		Vector *line = ed_cur_line(ed);
		if(ed->Sel.C[1].X == 0)
		{
			if(ed->Sel.C[1].Y > 0)
			{
				/* merge with previous line */
				Vector *prev = ed_line_get(ed, --ed->Sel.C[1].Y);
				ed->Sel.C[1].X = vector_len(prev);
				vector_push(prev, vector_len(line), vector_data(line));
				ed_line_remove(ed, ed->Sel.C[1].Y + 1);
			}
		}
		else
		{
			/* delete prev char */
			vector_remove(line, --ed->Sel.C[1].X, 1);
		}

		ed->CursorSaveX = -1;
		ed_sel_to_cursor(ed);
	}

	ed_render(ed);
}

static void ed_delete(Editor *ed)
{
	if(sel_wide(&ed->Sel))
	{
		ed_sel_delete(ed);
	}
	else
	{
		Vector *line = ed_cur_line(ed);
		u32 line_len = vector_len(line);
		if(ed->Sel.C[1].X >= line_len)
		{
			u32 num_lines = ed_num_lines(ed);
			ed->Sel.C[1].X = line_len;
			if(ed->Sel.C[1].Y < num_lines - 1)
			{
				/* merge with next line */
				u32 next_idx = ed->Sel.C[1].Y + 1;
				Vector *next = ed_line_get(ed, next_idx);
				vector_push(line, vector_len(next), vector_data(next));
				ed_line_remove(ed, next_idx);
			}
		}
		else
		{
			/* delete next char */
			vector_remove(line, ed->Sel.C[1].X, 1);
		}

		ed->CursorSaveX = -1;
	}

	ed_render(ed);
}

static void ed_char(Editor *ed, u32 chr)
{
	u8 ins[1];
	ed_sel_clear(ed);
	ins[0] = chr;
	vector_insert(ed_cur_line(ed), ed->Sel.C[1].X, 1, ins);
	++ed->Sel.C[1].X;
	ed->CursorSaveX = -1;
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_enter(Editor *ed)
{
	u32 len;
	char *str;
	Vector new, *cur;

	ed_sel_clear(ed);

	cur = ed_cur_line(ed);
	str = (char *)vector_data(cur) + ed->Sel.C[1].X;
	len = vector_len(cur) - ed->Sel.C[1].X;

	/* Copy characters after cursor on current line to new line */
	vector_init(&new, len);
	vector_push(&new, len, str);

	/* Insert new line */
	ed_line_insert(ed, ed->Sel.C[1].Y + 1, &new);

	/* Remove characters after cursor on current line */
	vector_remove(cur, ed->Sel.C[1].X, len);

	++ed->Sel.C[1].Y;
	ed->Sel.C[1].X = 0;
	ed->CursorSaveX = 0;
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_enter_after(Editor *ed)
{
	Vector new;
	vector_init(&new, 8);
	ed_line_insert(ed, ++ed->Sel.C[1].Y, &new);
	ed->Sel.C[1].X = 0;
	ed->CursorSaveX = 0;
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_enter_before(Editor *ed)
{
	Vector new;
	vector_init(&new, 8);
	ed_line_insert(ed, ed->Sel.C[1].Y, &new);
	ed->Sel.C[1].X = 0;
	ed->CursorSaveX = 0;
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_home_internal(Editor *ed)
{
	Vector *line = ed_cur_line(ed);
	char *buf = vector_data(line);
	u32 len = vector_len(line);
	u32 i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	ed->Sel.C[1].X = (ed->Sel.C[1].X == i) ? 0 : i;
	ed->CursorSaveX = -1;
}

static void ed_sel_home(Editor *ed)
{
	ed_home_internal(ed);
	ed_render(ed);
}

static void ed_home(Editor *ed)
{
	ed_home_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_end_internal(Editor *ed)
{
	ed->Sel.C[1].X = ed_cur_line_len(ed);
	ed->CursorSaveX = -1;
}

static void ed_sel_end(Editor *ed)
{
	ed_end_internal(ed);
	ed_render(ed);
}

static void ed_end(Editor *ed)
{
	ed_end_internal(ed);
	ed_sel_to_cursor(ed);
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
		ed->CursorSaveX = ed_cursor_pos_x(ed, prev_y, ed->Sel.C[1].X);
	}

	max_x = ed->CursorSaveX;
	for(i = 0, x = 0; i < len && x < max_x; ++i)
	{
		x = ed_chr_x_inc(ed, buf[i], x);
	}

	ed->Sel.C[1].X = i;
}

static void ed_up_internal(Editor *ed)
{
	if(ed->Sel.C[1].Y == 0)
	{
		ed->Sel.C[1].X = 0;
		ed->CursorSaveX = 0;
	}
	else
	{
		u32 prev_y = ed->Sel.C[1].Y;
		--ed->Sel.C[1].Y;
		ed_move_vertical(ed, prev_y);
	}
}

static void ed_sel_up(Editor *ed)
{
	ed_up_internal(ed);
	ed_render(ed);
}

static void ed_up(Editor *ed)
{
	ed_up_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_down_internal(Editor *ed)
{
	if(ed->Sel.C[1].Y >= ed_num_lines(ed) - 1)
	{
		ed->Sel.C[1].X = ed_cur_line_len(ed);
		ed->CursorSaveX = -1;
	}
	else
	{
		u32 prev_y = ed->Sel.C[1].Y;
		++ed->Sel.C[1].Y;
		ed_move_vertical(ed, prev_y);
	}
}

static void ed_sel_down(Editor *ed)
{
	ed_down_internal(ed);
	ed_render(ed);
}

static void ed_down(Editor *ed)
{
	ed_down_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_page_up_internal(Editor *ed)
{
	if(ed->Sel.C[1].Y >= ed->PageH)
	{
		u32 prev_y = ed->Sel.C[1].Y;
		ed->Sel.C[1].Y -= ed->PageH;
		ed_move_vertical(ed, prev_y);
	}
	else
	{
		ed->Sel.C[1].Y = 0;
		ed->Sel.C[1].X = 0;
		ed->CursorSaveX = 0;
	}
}

static void ed_sel_page_up(Editor *ed)
{
	ed_page_up_internal(ed);
	ed_render(ed);
}

static void ed_page_up(Editor *ed)
{
	ed_page_up_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_page_down_internal(Editor *ed)
{
	u32 num_lines = ed_num_lines(ed);
	u32 prev_y = ed->Sel.C[1].Y;
	ed->Sel.C[1].Y += ed->PageH;
	if(ed->Sel.C[1].Y >= num_lines)
	{
		ed->Sel.C[1].Y = num_lines - 1;
		ed->Sel.C[1].X = ed_cur_line_len(ed);
		ed->CursorSaveX = -1;
	}
	else
	{
		ed_move_vertical(ed, prev_y);
	}
}

static void ed_sel_page_down(Editor *ed)
{
	ed_page_down_internal(ed);
	ed_render(ed);
}

static void ed_page_down(Editor *ed)
{
	ed_page_down_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_left_internal(Editor *ed)
{
	if(ed->Sel.C[1].X == 0)
	{
		if(ed->Sel.C[1].Y > 0)
		{
			--ed->Sel.C[1].Y;
			ed->Sel.C[1].X = ed_cur_line_len(ed);
		}
	}
	else
	{
		--ed->Sel.C[1].X;
	}

	ed->CursorSaveX = -1;
}

static void ed_sel_left(Editor *ed)
{
	ed_left_internal(ed);
	ed_render(ed);
}

static void ed_left(Editor *ed)
{
	if(sel_wide(&ed->Sel))
	{
		sel_left(&ed->Sel);
	}
	else
	{
		ed_left_internal(ed);
		ed_sel_to_cursor(ed);
	}

	ed_render(ed);
}

static void ed_right_internal(Editor *ed)
{
	if(ed->Sel.C[1].X == ed_cur_line_len(ed))
	{
		if(ed->Sel.C[1].Y < ed_num_lines(ed) - 1)
		{
			++ed->Sel.C[1].Y;
			ed->Sel.C[1].X = 0;
		}
	}
	else
	{
		++ed->Sel.C[1].X;
	}

	ed->CursorSaveX = -1;
}

static void ed_sel_right(Editor *ed)
{
	ed_right_internal(ed);
	ed_render(ed);
}

static void ed_right(Editor *ed)
{
	if(sel_wide(&ed->Sel))
	{
		sel_right(&ed->Sel);
	}
	else
	{
		ed_right_internal(ed);
		ed_sel_to_cursor(ed);
	}

	ed_render(ed);
}

static void ed_top_internal(Editor *ed)
{
	ed->Sel.C[1].X = 0;
	ed->Sel.C[1].Y = 0;
	ed->CursorSaveX = 0;
}

static void ed_sel_top(Editor *ed)
{
	ed_top_internal(ed);
	ed_render(ed);
}

static void ed_top(Editor *ed)
{
	ed_top_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_bottom_internal(Editor *ed)
{
	ed->Sel.C[1].Y = ed_num_lines(ed) - 1;
	ed->Sel.C[1].X = ed_cur_line_len(ed);
	ed->CursorSaveX = -1;
}

static void ed_sel_bottom(Editor *ed)
{
	ed_bottom_internal(ed);
	ed_render(ed);
}

static void ed_bottom(Editor *ed)
{
	ed_bottom_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_reset_cursor(Editor *ed)
{
	ed->PageY = 0;
	ed->CursorSaveX = 0;
	memset(&ed->Sel, 0, sizeof(Selection));
}

static void ed_gotoxy(Editor *ed, u32 x, u32 y)
{
	u32 num_lines = ed_num_lines(ed);
	if(y >= num_lines)
	{
		y = num_lines - 1;
	}

	ed->Sel.C[1].Y = y;
	if(ed->Sel.C[1].Y > ed->PageH / 2)
	{
		ed->PageY = ed->Sel.C[1].Y - ed->PageH / 2;
	}

	ed->Sel.C[1].X = x;
	ed_sel_to_cursor(ed);
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

	ed->Language = LANGUAGE_C;
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
	u32 len;
	char *text = ed_sel_get(ed, &ed->Sel, &len);
	clipboard_store(text);
	_free(text);
}

static void ed_sel_all(Editor *ed)
{
	u32 last_line = ed_num_lines(ed) - 1;
	ed->Sel.C[0].X = 0;
	ed->Sel.C[0].Y = 0;
	ed->Sel.C[1].X = ed_line_len(ed, last_line);
	ed->Sel.C[1].Y = last_line;
	ed_render(ed);
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
	char *text = clipboard_load();
	ed_insert(ed, text);
	free(text);
	ed_render(ed);
}

static void ed_prev_word_internal(Editor *ed)
{
	if(ed->Sel.C[1].X == 0)
	{
		if(ed->Sel.C[1].Y > 0)
		{
			--ed->Sel.C[1].Y;
			ed->Sel.C[1].X = ed_cur_line_len(ed);
		}
	}
	else
	{
		if(ed->Sel.C[1].X > 0)
		{
			char *buf = vector_data(ed_cur_line(ed));
			u32 i = ed->Sel.C[1].X - 1;
			u32 type = char_type(buf[i]);
			while(i > 0 && char_type(buf[i - 1]) == type) { --i; }
			ed->Sel.C[1].X = i;
		}
	}

	ed->CursorSaveX = -1;
}

static void ed_sel_prev_word(Editor *ed)
{
	ed_prev_word_internal(ed);
	ed_render(ed);
}

static void ed_prev_word(Editor *ed)
{
	ed_prev_word_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_del_prev_word(Editor *ed)
{
	ed_prev_word_internal(ed);
	ed_sel_clear(ed);
	ed_render(ed);
}

static void ed_next_word_internal(Editor *ed)
{
	u32 len = ed_cur_line_len(ed);
	if(ed->Sel.C[1].X == len)
	{
		if(ed->Sel.C[1].Y < ed_num_lines(ed) - 1)
		{
			++ed->Sel.C[1].Y;
			ed->Sel.C[1].X = 0;
		}
	}
	else
	{
		if(ed->Sel.C[1].X < len)
		{
			char *buf = vector_data(ed_cur_line(ed));
			u32 i = ed->Sel.C[1].X;
			u32 type = char_type(buf[i]);
			while(i < len && char_type(buf[i]) == type) { ++i; }
			ed->Sel.C[1].X = i;
		}
	}

	ed->CursorSaveX = -1;
}

static void ed_sel_next_word(Editor *ed)
{
	ed_next_word_internal(ed);
	ed_render(ed);
}

static void ed_next_word(Editor *ed)
{
	ed_next_word_internal(ed);
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_del_next_word(Editor *ed)
{
	ed_next_word_internal(ed);
	ed_sel_clear(ed);
	ed_render(ed);
}

static void ed_del_cur_line(Editor *ed)
{
	u32 count = ed_num_lines(ed);
	if(count == 1)
	{
		ed_line_get(ed, 0)->Length = 0;
	}
	else
	{
		ed_line_remove(ed, ed->Sel.C[1].Y);
		if(ed->Sel.C[1].Y >= count - 1)
		{
			ed->Sel.C[1].Y = count - 2;
		}
	}

	ed->Sel.C[1].X = 0;
	ed->CursorSaveX = 0;
	ed_sel_to_cursor(ed);
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

static void ed_toggle_lang(Editor *ed)
{
	ed->Language = !ed->Language;
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

static void ed_ins(Editor *ed, const char *s, u32 len, u32 inc)
{
	ed_sel_clear(ed);
	vector_insert(ed_cur_line(ed), ed->Sel.C[1].X, len, s);
	ed->Sel.C[1].X += inc;
	ed->CursorSaveX = -1;
	ed_sel_to_cursor(ed);
	ed_render(ed);
}

static void ed_ins_comment(Editor *ed)
{
	static const char *ins = "/*  */";
	ed_ins(ed, ins, 6, 3);
}

static void ed_include(Editor *ed)
{
	static const char *ins = "#include \"\"";
	ed_ins(ed, ins, 11, 10);
}

static void ed_include_lib(Editor *ed)
{
	static const char *ins = "#include <>";
	ed_ins(ed, ins, 11, 10);
}

static void ed_sel_cur_line(Editor *ed)
{
	u32 last_line = ed_num_lines(ed) - 1;
	ed->Sel.C[0].X = 0;
	if(ed->Sel.C[1].Y < last_line)
	{
		ed->Sel.C[1].X = 0;
		++ed->Sel.C[1].Y;
	}
	else
	{
		u32 last_line_len = ed_line_len(ed, last_line);
		if(ed->Sel.C[1].X == last_line_len)
		{
			return;
		}

		ed->Sel.C[1].X = last_line_len;
	}

	ed_render(ed);
}

static void ed_new_file(Editor *ed)
{

}

static void ed_close_file(Editor *ed)
{

}

static void ed_quit(Editor *ed)
{

}

#include "nav.c"

static void ed_key_press_default(Editor *ed, u32 key, u32 cp)
{
	switch(key)
	{
	case MOD_CTRL | MOD_SHIFT | KEY_LEFT:   ed_sel_prev_word(ed);  break;
	case MOD_CTRL | KEY_LEFT:               ed_prev_word(ed);      break;
	case MOD_SHIFT | KEY_LEFT:              ed_sel_left(ed);       break;
	case KEY_LEFT:                          ed_left(ed);           break;
	case MOD_CTRL | MOD_SHIFT | KEY_RIGHT:  ed_sel_next_word(ed);  break;
	case MOD_CTRL | KEY_RIGHT:              ed_next_word(ed);      break;
	case MOD_SHIFT | KEY_RIGHT:             ed_sel_right(ed);      break;
	case KEY_RIGHT:                         ed_right(ed);          break;
	case MOD_CTRL | KEY_UP:                 ed_move_up(ed);        break;
	case MOD_SHIFT | KEY_UP:                ed_sel_up(ed);         break;
	case KEY_UP:                            ed_up(ed);             break;
	case MOD_CTRL | KEY_DOWN:               ed_move_down(ed);      break;
	case MOD_SHIFT | KEY_DOWN:              ed_sel_down(ed);       break;
	case KEY_DOWN:                          ed_down(ed);           break;
	case MOD_SHIFT | KEY_PAGE_UP:           ed_sel_page_up(ed);    break;
	case KEY_PAGE_UP:                       ed_page_up(ed);        break;
	case MOD_SHIFT | KEY_PAGE_DOWN:         ed_sel_page_down(ed);  break;
	case KEY_PAGE_DOWN:                     ed_page_down(ed);      break;
	case MOD_CTRL | MOD_SHIFT | KEY_HOME:   ed_sel_top(ed);        break;
	case MOD_CTRL | KEY_HOME:               ed_top(ed);            break;
	case MOD_SHIFT | KEY_HOME:              ed_sel_home(ed);       break;
	case KEY_HOME:                          ed_home(ed);           break;
	case MOD_CTRL | MOD_SHIFT | KEY_END:    ed_sel_bottom(ed);     break;
	case MOD_CTRL | KEY_END:                ed_bottom(ed);         break;
	case MOD_SHIFT | KEY_END:               ed_sel_end(ed);        break;
	case KEY_END:                           ed_end(ed);            break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                        ed_enter(ed);          break;
	case MOD_CTRL | KEY_RETURN:             ed_enter_after(ed);    break;
	case MOD_CTRL | MOD_SHIFT | KEY_RETURN: ed_enter_before(ed);   break;
	case MOD_CTRL | KEY_BACKSPACE:          ed_del_prev_word(ed);  break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:                     ed_backspace(ed);      break;
	case MOD_CTRL | KEY_DELETE:             ed_del_next_word(ed);  break;
	case MOD_SHIFT | KEY_DELETE:            ed_del_cur_line(ed);   break;
	case KEY_DELETE:                        ed_delete(ed);         break;
	case MOD_CTRL | KEY_L:                  ed_sel_cur_line(ed);   break;
	case MOD_CTRL | MOD_SHIFT | KEY_L:      ed_toggle_line_nr(ed); break;
	case MOD_CTRL | KEY_K:                  ed_toggle_lang(ed);    break;
	case MOD_CTRL | KEY_G:                  ed_goto(ed);           break;
	case MOD_CTRL | KEY_O:                  ed_open(ed);           break;
	case MOD_CTRL | KEY_C:                  ed_copy(ed);           break;
	case MOD_CTRL | KEY_X:                  ed_cut(ed);            break;
	case MOD_CTRL | KEY_V:                  ed_paste(ed);          break;
	case MOD_CTRL | KEY_S:                  ed_save(ed);           break;
	case MOD_CTRL | MOD_SHIFT | KEY_T:      ed_tab_size(ed);       break;
	case MOD_CTRL | KEY_T:
	case MOD_CTRL | KEY_N:                  ed_new_file(ed);       break;
	case MOD_CTRL | KEY_W:                  ed_close_file(ed);     break;
	case MOD_CTRL | KEY_Q:                  ed_quit(ed);           break;
	case MOD_CTRL | KEY_J:                  ed_whitespace(ed);     break;
	case MOD_CTRL | KEY_I:                  ed_include(ed);        break;
	case MOD_CTRL | MOD_SHIFT | KEY_I:      ed_include_lib(ed);    break;
	case MOD_CTRL | KEY_A:                  ed_sel_all(ed);        break;
	case MOD_CTRL | MOD_SHIFT | KEY_A:      ed_ins_comment(ed);    break;
	default:
		if(isprint(cp) || cp == '\t')
		{
			ed_char(ed, cp);
		}
		break;
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
