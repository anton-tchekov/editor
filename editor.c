#include "helpers.c"
#include "vector.c"
#include "keyword.c"
#include "textbuffer.c"
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

#define MAX_SEARCH_LEN 256

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
	char Path[128];
	char FileName[128];
	char Search[MAX_SEARCH_LEN];
	u32 SCursor, SLen;
} Editor;

typedef void (*EditorFunc)(Editor *);

typedef struct
{
	u16 Key;
	EditorFunc Event;
} EditorKeyBind;

static u32 editor_num_lines(Editor *ed)
{
	return vector_len(&ed->Lines);
}

static Vector *line_get(Editor *ed, size_t i)
{
	return (Vector *)vector_get(&ed->Lines, i);
}

static Vector *current_line(Editor *ed)
{
	return line_get(ed, ed->CursorY);
}

static u32 current_line_len(Editor *ed)
{
	return vector_len(current_line(ed));
}

static void editor_render_line_numbers(Editor *ed)
{
	char lnr_buf[8];
	u32 x, y, def, cur, color, lnr, lines, lnr_max, lnr_width;

	def = screen_color(COLOR_TABLE_LINE_NUMBER, COLOR_TABLE_BG);
	cur = screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG);

	lines = ed->Lines.Length;
	lnr = ed->PageY;
	lnr_max = lnr + ed->PageH;
	lnr_max = lnr_max < lines ? lnr_max : lines;
	lnr_width = dec_digit_cnt(lnr_max);

	for(y = 0; y < ed->PageH; ++y)
	{
		color = (lnr == ed->CursorY) ? cur : def;
		++lnr;
		if(lnr <= lines)
		{
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

static u32 _syntax_highlight(Editor *ed, u32 len, const char *line, u16 *render)
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
			if(c == '0')
			{
				render[x++] = screen_pack(c,
					screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
				++i;
				c = line[i];
				if(c == 'x' || c == 'X')
				{
					/* Hex */
					for(; i < len && isxdigit(c = line[i]); ++i)
					{
						render[x++] = screen_pack(c,
							screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
					}
				}
				else if(c == 'b' || c == 'B')
				{
					/* Binary */
					for(; i < len && is_bin(c = line[i]); ++i)
					{
						render[x++] = screen_pack(c,
							screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
					}
				}
				else
				{
					/* Octal */
					for(; i < len && isdigit(c = line[i]); ++i)
					{
						render[x++] = screen_pack(c,
							screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
					}
				}
			}
			else
			{
				/* Decimal */
				for(; i < len && isdigit(c = line[i]); ++i)
				{
					render[x++] = screen_pack(c,
						screen_color(COLOR_TABLE_NUMBER, COLOR_TABLE_BG));
				}
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
	const char *line = vector_data(vector_get(&ed->Lines, ed->CursorY));

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

static void _render_line(Editor *ed, u32 y, u32 cursor_x)
{
	u32 x, line, len, render_len, pack;
	u16 render[1024]; /* FIXME: Crash when line is longer than 256 chars */
	const char *text;

	line = ed->PageY + y;
	if(line >= editor_num_lines(ed))
	{
		len = 0;
	}
	else
	{
		Vector *lv = vector_get(&ed->Lines, line);
		len = vector_len(lv);
		text = vector_data(lv);
	}

	pack = screen_pack(' ', screen_color(COLOR_TABLE_FG, COLOR_TABLE_BG));
	render_len = _syntax_highlight(ed, len, text, render);
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

static void render_msg(Editor *ed)
{
	u32 x, y, c, color;
	const char *s;
	y = ed->PageH - 1;
	color = screen_color(COLOR_TABLE_FG,
		ed->MsgType ? COLOR_TABLE_ERROR : COLOR_TABLE_INFO);

	for(s = ed->Search, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, c, color);
	}

	for(; x < ed->FullW; ++x)
	{
		screen_set(x, y, ' ', color);
	}
}

static void _render_goto_line(Editor *ed)
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
}

static void editor_render(Editor *ed)
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
		u32 lines = editor_num_lines(ed);
		if(lines > ed->PageH && ed->PageY + ed->PageH >= lines)
		{
			ed->PageY = lines - ed->PageH;
		}
	}

	if(ed->ShowLineNr)
	{
		editor_render_line_numbers(ed);
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
			start_y = 1;
			_render_goto_line(ed);
		}
		else if(ed->Mode == EDITOR_MODE_MSG)
		{
			--end_y;
			render_msg(ed);
		}

		cursor_x = cursor_pos_x(ed);
		for(y = start_y; y < end_y; ++y)
		{
			_render_line(ed, y, cursor_x);
		}
	}
}

static void editor_msg(Editor *ed, u32 msg_type, const char *msg, ...)
{
	va_list args;
	ed->Mode = EDITOR_MODE_MSG;
	ed->MsgType = msg_type;
	va_start(args, msg);
	vsnprintf(ed->Search, sizeof(ed->Search), msg, args);
	va_end(args);
	editor_render(ed);
}

static void editor_backspace(Editor *ed)
{
	Vector *line;

	line = vector_get(&ed->Lines, ed->CursorY);
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			/* merge with previous line */
			Vector *prev = vector_get(&ed->Lines, --ed->CursorY);
			ed->CursorX = vector_len(prev);
			vector_push_range(prev, vector_len(line), line->Data);
			vector_destroy(line);
			vector_remove(&ed->Lines, ed->CursorY + 1);
		}
	}
	else
	{
		/* delete prev char */
		vector_remove(line, --ed->CursorX);
	}

	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_delete(Editor *ed)
{
	Vector *line = current_line(ed);
	u32 line_len = vector_len(line);

	if(ed->CursorX >= line_len)
	{
		u32 num_lines = editor_num_lines(ed);
		ed->CursorX = line_len;
		if(ed->CursorY < num_lines - 1)
		{
			u32 next_idx = ed->CursorY + 1;
			Vector *next = vector_get(&ed->Lines, next_idx);

			/* merge with next line */
			vector_push_range(line, vector_len(next), next->Data);
			vector_destroy(next);
			vector_remove(&ed->Lines, next_idx);
		}
	}
	else
	{
		/* delete next char */
		vector_remove(line, ed->CursorX);
	}

	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_char(Editor *ed, u32 chr)
{
	char c = chr;
	vector_insert(vector_get(&ed->Lines, ed->CursorY), ed->CursorX, &c);
	++ed->CursorX;
	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_enter(Editor *ed)
{
	Vector new;
	Vector *cur = current_line(ed);
	char *str = (char *)cur->Data + ed->CursorX;
	u32 len = vector_len(cur) - ed->CursorX;

	/* Copy characters after cursor on current line to new line */
	vector_init(&new, sizeof(char), len);
	vector_push_range(&new, len, str);

	/* Insert new line */
	vector_insert(&ed->Lines, ed->CursorY + 1, &new);

	/* Remove characters after cursor on current line */
	vector_remove_range(cur, ed->CursorX, len);

	++ed->CursorY;
	ed->CursorX = 0;
	ed->CursorSaveX = 0;
	editor_render(ed);
}

static void editor_home(Editor *ed)
{
	u32 i = 0;
	char *buf = current_line(ed)->Data;
	while(isspace(buf[i])) { ++i; }
	ed->CursorX = (ed->CursorX == i) ? 0 : i;
	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_end(Editor *ed)
{
	ed->CursorX = vector_len(vector_get(&ed->Lines, ed->CursorY));
	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void move_vertical(Editor *ed)
{
	u32 len = current_line_len(ed);
	ed->CursorX = ed->CursorSaveX;
	if(ed->CursorX > len)
	{
		ed->CursorX = len;
	}
}

static void editor_page_up(Editor *ed)
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

	move_vertical(ed);
	editor_render(ed);
}

static void editor_page_down(Editor *ed)
{
	u32 num_lines = editor_num_lines(ed);
	ed->CursorY += ed->PageH;
	if(ed->CursorY >= num_lines)
	{
		ed->CursorY = num_lines - 1;
		ed->CursorX = current_line_len(ed);
		ed->CursorSaveX = ed->CursorX;
	}

	move_vertical(ed);
	editor_render(ed);
}

static void editor_left(Editor *ed)
{
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			--ed->CursorY;
			ed->CursorX = current_line_len(ed);
		}
	}
	else
	{
		--ed->CursorX;
	}

	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_right(Editor *ed)
{
	if(ed->CursorX == current_line_len(ed))
	{
		if(ed->CursorY < editor_num_lines(ed) - 1)
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
	editor_render(ed);
}

static void editor_up(Editor *ed)
{
	if(ed->CursorY == 0)
	{
		ed->CursorX = 0;
		ed->CursorSaveX = 0;
	}
	else
	{
		--ed->CursorY;
		move_vertical(ed);
	}

	editor_render(ed);
}

static void editor_down(Editor *ed)
{
	if(ed->CursorY >= editor_num_lines(ed) - 1)
	{
		ed->CursorX = current_line_len(ed);
		ed->CursorSaveX = ed->CursorX;
	}
	else
	{
		++ed->CursorY;
		move_vertical(ed);
	}

	editor_render(ed);
}

static void editor_top(Editor *ed)
{
	ed->CursorX = 0;
	ed->CursorY = 0;
	ed->CursorSaveX = 0;
	editor_render(ed);
}

static void editor_bottom(Editor *ed)
{
	ed->CursorY = editor_num_lines(ed) - 1;
	ed->CursorX = current_line_len(ed);
	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_reset_cursor(Editor *ed)
{
	ed->PageY = 0;
	ed->PageX = 0;

	ed->CursorX = 0;
	ed->CursorSaveX = 0;
	ed->CursorY = 0;
}

static void editor_init(Editor *ed, u32 width, u32 height)
{
	Vector first_line;
	vector_init(&ed->Lines, sizeof(Vector), 128);
	vector_init(&first_line, sizeof(char), 8);
	vector_push(&ed->Lines, &first_line);

	editor_reset_cursor(ed);

	ed->TabSize = 4;
	ed->ShowWhitespace = 1;

	ed->FullW = width;
	ed->PageH = height;

	ed->ShowLineNr = 1;
}

static void editor_goto(Editor *ed)
{
	ed->Mode = EDITOR_MODE_GOTO;
	ed->SLen = 0;
	ed->SCursor = 0;
	editor_render(ed);
	(void)ed;
}

static void selection_store(Editor *ed)
{
	(void)ed;
}

static void selection_delete(Editor *ed)
{
	(void)ed;
}

static void editor_cut(Editor *ed)
{
	selection_store(ed);
	selection_delete(ed);
	editor_render(ed);
}

static void editor_copy(Editor *ed)
{
	selection_store(ed);
	editor_render(ed);
}

static void editor_paste(Editor *ed)
{
	/* editor_insert(ed, clipboard_load()); */
	editor_render(ed);
}

static void editor_prev_word(Editor *ed)
{
	if(ed->CursorX == 0)
	{
		if(ed->CursorY > 0)
		{
			--ed->CursorY;
			ed->CursorX = current_line_len(ed);
		}
	}
	else
	{
		if(ed->CursorX > 0)
		{
			char *buf = current_line(ed)->Data;
			u32 i = ed->CursorX - 1;
			u32 kind = char_type(buf[i]);
			while(i > 0 && char_type(buf[i - 1]) == kind) { --i; }
			ed->CursorX = i;
		}
	}

	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_next_word(Editor *ed)
{
	u32 len = current_line_len(ed);
	if(ed->CursorX == len)
	{
		if(ed->CursorY < editor_num_lines(ed) - 1)
		{
			++ed->CursorY;
			ed->CursorX = 0;
		}
	}
	else
	{
		if(ed->CursorX < len)
		{
			char *buf = current_line(ed)->Data;
			u32 i = ed->CursorX;
			u32 kind = char_type(buf[i]);
			while(i < len && char_type(buf[i]) == kind) { ++i; }
			ed->CursorX = i;
		}
	}

	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_move_up(Editor *ed)
{
	if(ed->PageY > 0)
	{
		--ed->PageY;
	}

	editor_render(ed);
}

static void editor_move_down(Editor *ed)
{
	++ed->PageY;
	editor_render(ed);
}

static void editor_toggle_line_nr(Editor *ed)
{
	ed->ShowLineNr = !ed->ShowLineNr;
	editor_render(ed);
}

static void editor_tab_size(Editor *ed)
{
	ed->TabSize <<= 1;
	if(ed->TabSize > 8)
	{
		ed->TabSize = 2;
	}

	editor_render(ed);
}

static void editor_whitespace(Editor *ed)
{
	ed->ShowWhitespace = !ed->ShowWhitespace;
	editor_render(ed);
}

static void editor_clear(Editor *ed)
{
	size_t i;
	for(i = 0; i < ed->Lines.Length; ++i)
	{
		vector_destroy(line_get(ed, i));
	}

	vector_clear(&ed->Lines);
}

static void editor_load_text(Editor *ed, const char *buf, size_t len)
{
	u32 c;
	Vector line;
	const char *p, *end;

	editor_clear(ed);
	editor_reset_cursor(ed);
	vector_init(&line, sizeof(char), 8);
	for(p = buf, end = buf + len; p < end; ++p)
	{
		c = *p;
		if(c == '\n')
		{
			vector_push(&ed->Lines, &line);
			vector_init(&line, sizeof(char), 8);
		}
		else if(isprint(c) || c == '\t')
		{
			vector_push(&line, &c);
		}
	}

	vector_push(&ed->Lines, &line);
}

static void editor_load(Editor *ed, const char *filename)
{
	size_t len;
	char *buf;

	if(!(buf = file_read(filename, &len)))
	{
		editor_msg(ed, EDITOR_ERROR, "Failed to open file");
		return;
	}

	if(is_text(buf, len))
	{
		free(buf);
		editor_msg(ed, EDITOR_ERROR, "Invalid character, binary file?");
		return;
	}

	ed->Mode = EDITOR_MODE_DEFAULT;
	strcpy(ed->FileName, filename);
	editor_load_text(ed, buf, len);
	free(buf);
	editor_render(ed);
}

static size_t editor_count_bytes(Editor *ed)
{
	size_t i, len;

	assert(ed->Lines.Length >= 1);

	for(i = 0, len = 0; i < ed->Lines.Length; ++i)
	{
		len += vector_len(vector_get(&ed->Lines, i)) + 1;
	}

	return len - 1;
}

static void editor_save(Editor *ed)
{
	size_t i, len, line_len;
	char *buf, *p;
	Vector *cur;

	len = editor_count_bytes(ed);
	p = buf = _malloc(len);
	for(i = 0; i < ed->Lines.Length; ++i)
	{
		cur = vector_get(&ed->Lines, i);
		line_len = vector_len(cur);
		memcpy(p, vector_data(cur), line_len);
		p += line_len;
		if(i < ed->Lines.Length - 1)
		{
			*p++ = '\n';
		}
	}

	if(file_write(ed->FileName, buf, len))
	{
		editor_msg(ed, EDITOR_ERROR, "Writing file failed");
	}
	else
	{
		editor_msg(ed, EDITOR_INFO, "File saved");
	}

	_free(buf);
}

static void _ed_inc(Editor *ed, const char *s)
{
	vector_insert_range(vector_get(&ed->Lines, ed->CursorY),
		ed->CursorX, 11, s);
	ed->CursorX += 10;
	ed->CursorSaveX = ed->CursorX;
	editor_render(ed);
}

static void editor_include(Editor *ed)
{
	static const char *ins = "#include \"\"";
	_ed_inc(ed, ins);
}

static void editor_include_lib(Editor *ed)
{
	static const char *ins = "#include <>";
	_ed_inc(ed, ins);
}

static void editor_key_press_default(Editor *ed, u32 key, u32 cp)
{
	static EditorKeyBind key_events[] =
	{
		{ MOD_CTRL | KEY_LEFT,  editor_prev_word      },
		{ KEY_LEFT,             editor_left           },
		{ MOD_CTRL | KEY_RIGHT, editor_next_word      },
		{ KEY_RIGHT,            editor_right          },
		{ MOD_CTRL | KEY_UP,    editor_move_up        },
		{ KEY_UP,               editor_up             },
		{ MOD_CTRL | KEY_DOWN,  editor_move_down      },
		{ KEY_DOWN,             editor_down           },
		{ KEY_PAGE_UP,          editor_page_up        },
		{ KEY_PAGE_DOWN,        editor_page_down      },

		{ MOD_CTRL | KEY_HOME,  editor_top            },
		{ KEY_HOME,             editor_home           },
		{ MOD_CTRL | KEY_END,   editor_bottom         },
		{ KEY_END,              editor_end            },

		{ KEY_RETURN,           editor_enter          },
		{ KEY_BACKSPACE,        editor_backspace      },
		{ KEY_DELETE,           editor_delete         },

		{ MOD_CTRL | KEY_L,     editor_toggle_line_nr },
		{ MOD_CTRL | KEY_G,     editor_goto           },
		{ MOD_CTRL | KEY_C,     editor_copy           },
		{ MOD_CTRL | KEY_X,     editor_cut            },
		{ MOD_CTRL | KEY_V,     editor_paste          },
		{ MOD_CTRL | KEY_S,     editor_save           },

		{ MOD_CTRL | KEY_T,     editor_tab_size       },
		{ MOD_CTRL | KEY_J,     editor_whitespace     },

		{ MOD_CTRL | KEY_I,             editor_include     },
		{ MOD_CTRL | MOD_SHIFT | KEY_I, editor_include_lib },
	};

	size_t i;
	EditorKeyBind *kb;
	for(i = 0; i < ARRLEN(key_events); ++i)
	{
		kb = key_events + i;
		if(kb->Key == key)
		{
			kb->Event(ed);
			return;
		}
	}

	if(isprint(cp) || cp == '\t')
	{
		editor_char(ed, cp);
	}
}

static void editor_key_press_msg(Editor *ed, u32 key, u32 cp)
{
	if((ed->MsgType == EDITOR_ERROR && key == KEY_RETURN) ||
		(ed->MsgType == EDITOR_INFO))
	{
		ed->Mode = EDITOR_MODE_DEFAULT;
		editor_render(ed);
	}

	(void)cp;
}

#include "nav.c"

static void editor_key_press(Editor *ed, u32 key, u32 cp)
{
	typedef void (*KeyPressFN)(Editor *, u32, u32);
	static const KeyPressFN fns[EDITOR_MODE_COUNT] =
	{
		editor_key_press_default,
		editor_key_press_nav,
		editor_key_press_msg
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

	editor_key_press(&editor, key, cp);
}

static void event_resize(u32 w, u32 h)
{
	editor.FullW = w;
	editor.PageH = h;
	strcpy(editor.Path, "./");
	editor_render(&editor);
}

static void event_init(int argc, char *argv[], u32 w, u32 h)
{
#ifndef NDEBUG
	test_run_all();
#endif

	keyword_init();
	editor_init(&editor, w, h);

	if(argc == 2)
	{
		editor_load(&editor, argv[1]);
	}
}

static void event_exit(void)
{
	editor_clear(&editor);
	vector_destroy(&editor.Lines);
	print_mem();
}
