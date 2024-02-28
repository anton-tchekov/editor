typedef struct
{
	vector lines;
	selection sel;
	char *filename;
	u32 page_y;
	i32 cursor_save_x;
	u8 language;
	u8 modified;
	u8 exists;
} textbuf;

static u32 tb_num_lines(textbuf *t)
{
	return vector_len(&t->lines) / sizeof(vector);
}

static void tb_reset_cursor(textbuf *t)
{
	t->page_y = 0;
	t->cursor_save_x = 0;
	memset(&t->sel, 0, sizeof(t->sel));
}

static textbuf *tb_new(const char *name, const char *text,
	u32 on_disk, u32 lang)
{
	vector line;
	textbuf *t = _malloc(sizeof(textbuf));
	t->filename = _strdup(name);
	t->language = lang;
	t->modified = 0;
	t->exists = on_disk;
	tb_reset_cursor(t);
	if(text)
	{
		u32 c;
		const char *linestart, *p;
		vector_init(&t->lines, count_char(text, '\n') + 1);
		p = text;
		linestart = text;
		do
		{
			c = *p;
			if(c == '\0' || c == '\n')
			{
				vector_from(&line, linestart, p - linestart);
				vector_push(&t->lines, sizeof(line), &line);
				linestart = p + 1;
			}
			++p;
		}
		while(c != '\0');
	}
	else
	{
		vector_init(&t->lines, 32 * sizeof(vector));
		vector_init(&line, 8);
		vector_push(&t->lines, sizeof(line), &line);
	}

	return t;
}

static void tb_change_filename(textbuf *t, const char *name)
{
	_free(t->filename);
	t->filename = _strdup(name);
}

static vector *tb_get_line(textbuf *t, u32 i)
{
	return (vector *)vector_get(&t->lines, i * sizeof(vector));
}

static vector *tb_cur_line(textbuf *t)
{
	return tb_get_line(t, t->sel.c[1].y);
}

static u32 tb_line_len(textbuf *t, u32 i)
{
	return vector_len(tb_get_line(t, i));
}

static const char *tb_line_data(textbuf *t, u32 i)
{
	return vector_data(tb_get_line(t, i));
}

static u32 tb_cur_line_len(textbuf *t)
{
	return vector_len(tb_cur_line(t));
}

static const char *tb_cur_line_data(textbuf *t)
{
	return vector_data(tb_cur_line(t));
}

static void tb_sel_to_cursor(textbuf *t)
{
	t->sel.c[0] = t->sel.c[1];
}

static void tb_scroll_to_cursor(textbuf *t)
{
	if(t->sel.c[1].y < t->page_y)
	{
		t->page_y = t->sel.c[1].y;
	}

	if(t->sel.c[1].y >= t->page_y + _screen_height)
	{
		t->page_y = t->sel.c[1].y - _screen_height + 1;
	}
}

static void tb_ins_line(textbuf *t, u32 line, vector *v)
{
	vector_insert(&t->lines, line * sizeof(vector), sizeof(vector), v);
}

static void tb_ins_lines(textbuf *t, u32 y, u32 count)
{
	vector_makespace(&t->lines, y * sizeof(vector), count * sizeof(vector));
}

static void tb_remove_line(textbuf *t, u32 line)
{
	vector_destroy(tb_get_line(t, line));
	vector_remove(&t->lines, line * sizeof(vector), sizeof(vector));
}

static void tb_remove_lines(textbuf *t, u32 y1, u32 y2)
{
	u32 i;
	for(i = y1; i <= y2; ++i)
	{
		vector_destroy(tb_get_line(t, i));
	}

	vector_remove(&t->lines, y1 * sizeof(vector),
		(y2 - y1 + 1) * sizeof(vector));
}

static void tb_destroy(textbuf *t)
{
	u32 i, num_lines = tb_num_lines(t);
	for(i = 0; i < num_lines; ++i)
	{
		vector_destroy(tb_get_line(t, i));
	}

	vector_destroy(&t->lines);
	_free(t->filename);
	_free(t);
}

static void tb_sel_all(textbuf *t)
{
	u32 last_line = tb_num_lines(t) - 1;
	t->sel.c[0].x = 0;
	t->sel.c[0].y = 0;
	t->sel.c[1].x = tb_line_len(t, last_line);
	t->sel.c[1].y = last_line;
}

static void tb_sel_delete(textbuf *t)
{
	u32 x1, y1, x2, y2;
	vector *line;
	selection nsel = t->sel;
	sel_norm(&nsel);

	x1 = nsel.c[0].x;
	y1 = nsel.c[0].y;
	x2 = nsel.c[1].x;
	y2 = nsel.c[1].y;

	t->sel.c[1] = t->sel.c[0] = nsel.c[0];

	line = tb_get_line(t, y1);
	if(y1 == y2)
	{
		vector_remove(line, x1, x2 - x1);
	}
	else
	{
		vector *last = tb_get_line(t, y2);
		vector_replace(line, x1, vector_len(line) - x1,
			(u8 *)vector_data(last) + x2, vector_len(last) - x2);
		tb_remove_lines(t, y1 + 1, y2);
	}
}

static u32 tb_sel_count_bytes(textbuf *t, selection *sel)
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

	len = tb_line_len(t, y1) - x1 + 1;
	for(++y1; y1 < y2; ++y1)
	{
		len += tb_line_len(t, y1) + 1;
	}

	return len + x2;
}

static char *tb_sel_get(textbuf *t, u32 *out_len)
{
	u32 x1, y1, x2, y2, len;
	char *output, *p;
	selection nsel = t->sel;
	sel_norm(&nsel);

	x1 = nsel.c[0].x;
	y1 = nsel.c[0].y;
	x2 = nsel.c[1].x;
	y2 = nsel.c[1].y;

	*out_len = len = tb_sel_count_bytes(t, &nsel);
	p = output = _malloc(len + 1);
	if(y1 == y2)
	{
		memcpy(p, tb_line_data(t, y1) + x1, len);
		p += len;
	}
	else
	{
		vector *line = tb_get_line(t, y1);
		len = vector_len(line) - x1;
		memcpy(p, (u8 *)vector_data(line) + x1, len);
		p += len;
		for(++y1; y1 < y2; ++y1)
		{
			*p++ = '\n';
			line = tb_get_line(t, y1);
			len = vector_len(line);
			memcpy(p, vector_data(line), len);
			p += len;
		}

		*p++ = '\n';
		memcpy(p, tb_line_data(t, y1), x2);
		p += x2;
	}

	*p = '\0';
	return output;
}

static void tb_sel_clear(textbuf *t)
{
	if(sel_wide(&t->sel))
	{
		tb_sel_delete(t);
	}
}

static void tb_delete(textbuf *t)
{
	if(sel_wide(&t->sel))
	{
		tb_sel_delete(t);
	}
	else
	{
		vector *line = tb_cur_line(t);
		u32 line_len = vector_len(line);
		if(t->sel.c[1].x >= line_len)
		{
			u32 num_lines = tb_num_lines(t);
			t->sel.c[1].x = line_len;
			if(t->sel.c[1].y < num_lines - 1)
			{
				/* Merge with next line */
				u32 next_idx = t->sel.c[1].y + 1;
				vector *next = tb_get_line(t, next_idx);
				vector_push(line, vector_len(next), vector_data(next));
				tb_remove_line(t, next_idx);
			}
		}
		else
		{
			/* Delete next char */
			vector_remove(line, t->sel.c[1].x, 1);
		}

		t->cursor_save_x = -1;
	}

	tb_scroll_to_cursor(t);
}

static void tb_backspace(textbuf *t)
{
	if(sel_wide(&t->sel))
	{
		tb_sel_delete(t);
	}
	else
	{
		vector *line = tb_cur_line(t);
		if(t->sel.c[1].x == 0)
		{
			if(t->sel.c[1].y > 0)
			{
				/* Merge with previous line */
				vector *prev = tb_get_line(t, --t->sel.c[1].y);
				t->sel.c[1].x = vector_len(prev);
				vector_push(prev, vector_len(line), vector_data(line));
				tb_remove_line(t, t->sel.c[1].y + 1);
			}
		}
		else
		{
			/* Delete previous char */
			vector_remove(line, --t->sel.c[1].x, 1);
		}

		t->cursor_save_x = -1;
		tb_sel_to_cursor(t);
	}

	tb_scroll_to_cursor(t);
}

static void tb_char(textbuf *t, u32 c)
{
	u8 ins[1];
	tb_sel_clear(t);
	ins[0] = c;
	vector_insert(tb_cur_line(t), t->sel.c[1].x, 1, ins);
	++t->sel.c[1].x;
	t->cursor_save_x = -1;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_sel_home(textbuf *t)
{
	vector *line = tb_cur_line(t);
	char *buf = vector_data(line);
	u32 len = vector_len(line);
	u32 i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	t->sel.c[1].x = (t->sel.c[1].x == i) ? 0 : i;
	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_home(textbuf *t)
{
	tb_sel_home(t);
	tb_sel_to_cursor(t);
}

static void tb_enter_before(textbuf *t)
{
	vector new_line;
	vector_init(&new_line, 8);
	tb_ins_line(t, t->sel.c[1].y, &new_line);
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter_after(textbuf *t)
{
	vector new_line;
	vector_init(&new_line, 8);
	tb_ins_line(t, ++t->sel.c[1].y, &new_line);
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter(textbuf *t)
{
	u32 len;
	char *str;
	vector new_line, *cur;

	tb_sel_clear(t);

	cur = tb_cur_line(t);
	str = (char *)vector_data(cur) + t->sel.c[1].x;
	len = vector_len(cur) - t->sel.c[1].x;

	/* Copy characters after cursor on current line to new line */
	vector_init(&new_line, len);
	vector_push(&new_line, len, str);

	/* Insert new line */
	tb_ins_line(t, t->sel.c[1].y + 1, &new_line);

	/* Remove characters after cursor on current line */
	vector_remove(tb_cur_line(t), t->sel.c[1].x, len);

	++t->sel.c[1].y;
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_sel_end(textbuf *t)
{
	t->sel.c[1].x = tb_cur_line_len(t);
	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_end(textbuf *t)
{
	tb_sel_end(t);
	tb_sel_to_cursor(t);
}

static void tb_del_cur_line(textbuf *t)
{
	u32 count = tb_num_lines(t);
	if(count == 1)
	{
		tb_get_line(t, 0)->len = 0;
	}
	else
	{
		tb_remove_line(t, t->sel.c[1].y);
		if(t->sel.c[1].y >= count - 1)
		{
			t->sel.c[1].y = count - 2;
		}
	}

	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_toggle_lang(textbuf *t)
{
	if(++t->language == LANGUAGE_COUNT)
	{
		t->language = 0;
	}
}

static u32 tb_count_bytes(textbuf *t)
{
	u32 i, len, num_lines = tb_num_lines(t);
	for(i = 0, len = 0; i < num_lines; ++i)
	{
		len += tb_line_len(t, i) + 1;
	}

	return len - 1;
}

static char *tb_export(textbuf *t, u32 *len)
{
	u32 bytes = tb_count_bytes(t);
	char *buf = _malloc(bytes);
	char *p = buf;
	u32 i, num_lines = tb_num_lines(t);
	for(i = 0; i < num_lines; ++i)
	{
		vector *cur = tb_get_line(t, i);
		u32 line_len = vector_len(cur);
		memcpy(p, vector_data(cur), line_len);
		p += line_len;
		if(i < num_lines - 1)
		{
			*p++ = '\n';
		}
	}

	*len = bytes;
	return buf;
}

static void tb_ins(textbuf *t, const char *s, u32 len, u32 inc)
{
	tb_sel_clear(t);
	vector_insert(tb_cur_line(t), t->sel.c[1].x, len, s);
	t->sel.c[1].x += inc;
	t->cursor_save_x = -1;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_ins_comment(textbuf *t)
{
	tb_ins(t, "/*  */", 6, 3);
}

static void tb_ins_include(textbuf *t)
{
	tb_ins(t, "#include \"\"", 11, 10);
}

static void tb_ins_include_lib(textbuf *t)
{
	tb_ins(t, "#include <>", 11, 10);
}

static void tb_sel_cur_line(textbuf *t)
{
	u32 last_line = tb_num_lines(t) - 1;
	t->sel.c[0].x = 0;
	if(t->sel.c[1].y < last_line)
	{
		t->sel.c[1].x = 0;
		++t->sel.c[1].y;
	}
	else
	{
		u32 last_line_len = tb_line_len(t, last_line);
		if(t->sel.c[1].x == last_line_len)
		{
			return;
		}

		t->sel.c[1].x = last_line_len;
	}

	tb_scroll_to_cursor(t);
}

static void tb_trailing(textbuf *t)
{
	u32 i, len, end = tb_num_lines(t);
	for(i = 0; i < end; ++i)
	{
		vector *line = tb_get_line(t, i);
		char *data = vector_data(line);
		len = vector_len(line);
		while(len > 0 && isspace(data[len - 1]))
		{
			--len;
		}

		if(len != line->len)
		{
			t->modified = 1;
			line->len = len;
		}
	}

	len = tb_line_len(t, t->sel.c[0].y);
	if(t->sel.c[0].x > len)
	{
		t->sel.c[0].x = len;
	}

	len = tb_line_len(t, t->sel.c[1].y);
	if(t->sel.c[1].x > len)
	{
		t->cursor_save_x = -1;
		t->sel.c[1].x = len;
	}
}

static void tb_insert(textbuf *t, const char *text)
{
	u32 c, y, new_lines;
	const char *p, *s;

	tb_sel_clear(t);

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

	y = t->sel.c[1].y;
	if(!new_lines)
	{
		u32 len = s - text;
		vector_insert(tb_get_line(t, y), t->sel.c[1].x, len, text);
		t->sel.c[1].x += len;
	}
	else
	{
		char *last;
		u32 slen, rlen;
		vector line, *first, *lines;

		/* Insert new lines in advance to avoid quadratic complexity */
		tb_ins_lines(t, y + 1, new_lines);
		lines = vector_data(&t->lines);

		/* Last line */
		first = tb_get_line(t, y);
		rlen = vector_len(first) - t->sel.c[1].x;
		slen = s - p;
		vector_init(&line, rlen + slen);
		line.len = rlen + slen;
		last = vector_data(&line);
		memcpy(last, p, slen);
		memcpy(last + slen, (u8 *)vector_data(first) + t->sel.c[1].x, rlen);
		lines[y + new_lines] = line;

		first->len = t->sel.c[1].x;
		t->sel.c[1].x = slen;
		t->sel.c[1].y = y + new_lines;

		/* First line */
		p = strchr(text, '\n');
		vector_push(tb_get_line(t, y), p - text, text);
		text = p + 1;

		/* Other lines */
		while((p = strchr(text, '\n')))
		{
			vector_from(&line, text, p - text);
			lines[++y] = line;
			text = p + 1;
		}
	}

	t->cursor_save_x = -1;
	tb_sel_to_cursor(t);
	t->modified = 1;
}

static u32 tb_line_hidden(textbuf *t, u32 y)
{
	return (y >= t->page_y + _screen_height) || (y < t->page_y);
}

static void tb_gotoxy(textbuf *t, u32 x, u32 y)
{
	u32 num_lines = tb_num_lines(t);
	if(y >= num_lines)
	{
		y = num_lines - 1;
	}

	t->sel.c[1].y = y;
	if(tb_line_hidden(t, y))
	{
		t->page_y = y - _screen_height / 2;
	}

	t->sel.c[1].x = x;
	tb_sel_to_cursor(t);
	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_goto_def(textbuf *t, const char *s)
{
	u32 i;
	u32 len = tb_num_lines(t);
	u32 sl = strlen(s);
	for(i = 0; i < len; ++i)
	{
		vector *line = tb_get_line(t, i);
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

		tb_gotoxy(t, offset, i);
		return;
	}
}

static u32 tb_chr_x_inc(u32 c, u32 x)
{
	return (c == '\t') ?
		((x + tabsize) / tabsize * tabsize) :
		(x + 1);
}

static u32 tb_cursor_pos_x(textbuf *t, u32 y, u32 end)
{
	u32 i;
	u32 x = 0;
	const char *line = tb_line_data(t, y);
	for(i = 0; i < end; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return x;
}

static void tb_move_vertical(textbuf *t, u32 prev_y)
{
	vector *line = tb_cur_line(t);
	u32 len = vector_len(line);
	const char *buf = vector_data(line);
	u32 i, x, max_x;

	if(t->cursor_save_x < 0)
	{
		t->cursor_save_x = tb_cursor_pos_x(t, prev_y, t->sel.c[1].x);
	}

	max_x = t->cursor_save_x;
	for(i = 0, x = 0; i < len && x < max_x; ++i)
	{
		x = tb_chr_x_inc(buf[i], x);
	}

	t->sel.c[1].x = i;
}

static void tb_sel_up(textbuf *t)
{
	if(t->sel.c[1].y == 0)
	{
		t->sel.c[1].x = 0;
		t->cursor_save_x = 0;
	}
	else
	{
		u32 prev_y = t->sel.c[1].y;
		--t->sel.c[1].y;
		tb_move_vertical(t, prev_y);
	}

	tb_scroll_to_cursor(t);
}

static void tb_up(textbuf *t)
{
	tb_sel_up(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_down(textbuf *t)
{
	if(t->sel.c[1].y >= tb_num_lines(t) - 1)
	{
		t->sel.c[1].x = tb_cur_line_len(t);
		t->cursor_save_x = -1;
	}
	else
	{
		u32 prev_y = t->sel.c[1].y;
		++t->sel.c[1].y;
		tb_move_vertical(t, prev_y);
	}

	tb_scroll_to_cursor(t);
}

static void tb_down(textbuf *t)
{
	tb_sel_down(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_page_up(textbuf *t)
{
	if(t->sel.c[1].y >= _screen_height)
	{
		u32 prev_y = t->sel.c[1].y;
		t->sel.c[1].y -= _screen_height;
		tb_move_vertical(t, prev_y);
	}
	else
	{
		t->sel.c[1].y = 0;
		t->sel.c[1].x = 0;
		t->cursor_save_x = 0;
	}

	tb_scroll_to_cursor(t);
}

static void tb_page_up(textbuf *t)
{
	tb_sel_page_up(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_page_down(textbuf *t)
{
	u32 num_lines = tb_num_lines(t);
	u32 prev_y = t->sel.c[1].y;
	t->sel.c[1].y += _screen_height;
	if(t->sel.c[1].y >= num_lines)
	{
		t->sel.c[1].y = num_lines - 1;
		t->sel.c[1].x = tb_cur_line_len(t);
		t->cursor_save_x = -1;
	}
	else
	{
		tb_move_vertical(t, prev_y);
	}

	tb_scroll_to_cursor(t);
}

static void tb_page_down(textbuf *t)
{
	tb_sel_page_down(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_prev_word(textbuf *t)
{
	if(t->sel.c[1].x == 0)
	{
		if(t->sel.c[1].y > 0)
		{
			--t->sel.c[1].y;
			t->sel.c[1].x = tb_cur_line_len(t);
		}
	}
	else
	{
		if(t->sel.c[1].x > 0)
		{
			const char *buf = tb_cur_line_data(t);
			u32 i = t->sel.c[1].x - 1;
			u32 type = char_type(buf[i]);
			while(i > 0 && char_type(buf[i - 1]) == type) { --i; }
			t->sel.c[1].x = i;
		}
	}

	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_del_prev_word(textbuf *t)
{
	tb_sel_prev_word(t);
	tb_sel_clear(t);
	t->modified = 1;
}

static void tb_prev_word(textbuf *t)
{
	tb_sel_prev_word(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_next_word(textbuf *t)
{
	u32 len = tb_cur_line_len(t);
	if(t->sel.c[1].x == len)
	{
		if(t->sel.c[1].y < tb_num_lines(t) - 1)
		{
			++t->sel.c[1].y;
			t->sel.c[1].x = 0;
		}
	}
	else
	{
		if(t->sel.c[1].x < len)
		{
			const char *buf = tb_cur_line_data(t);
			u32 i = t->sel.c[1].x;
			u32 type = char_type(buf[i]);
			while(i < len && char_type(buf[i]) == type) { ++i; }
			t->sel.c[1].x = i;
		}
	}

	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_del_next_word(textbuf *t)
{
	tb_sel_next_word(t);
	tb_sel_clear(t);
	t->modified = 1;
}

static void tb_next_word(textbuf *t)
{
	tb_sel_next_word(t);
	tb_sel_to_cursor(t);
}

static void tb_move_up(textbuf *t)
{
	if(t->page_y > 0)
	{
		--t->page_y;
	}
}

static void tb_move_down(textbuf *t)
{
	++t->page_y;
}

static void tb_sel_left(textbuf *t)
{
	if(t->sel.c[1].x == 0)
	{
		if(t->sel.c[1].y > 0)
		{
			--t->sel.c[1].y;
			t->sel.c[1].x = tb_cur_line_len(t);
		}
	}
	else
	{
		--t->sel.c[1].x;
	}

	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_left(textbuf *t)
{
	if(sel_wide(&t->sel))
	{
		sel_left(&t->sel);
	}
	else
	{
		tb_sel_left(t);
		tb_sel_to_cursor(t);
	}

	tb_scroll_to_cursor(t);
}

static void tb_sel_right(textbuf *t)
{
	if(t->sel.c[1].x == tb_cur_line_len(t))
	{
		if(t->sel.c[1].y < tb_num_lines(t) - 1)
		{
			++t->sel.c[1].y;
			t->sel.c[1].x = 0;
		}
	}
	else
	{
		++t->sel.c[1].x;
	}

	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_right(textbuf *t)
{
	if(sel_wide(&t->sel))
	{
		sel_right(&t->sel);
	}
	else
	{
		tb_sel_right(t);
		tb_sel_to_cursor(t);
	}

	tb_scroll_to_cursor(t);
}

static void tb_sel_top(textbuf *t)
{
	cursor_zero(&t->sel.c[1]);
	t->cursor_save_x = 0;
	tb_scroll_to_cursor(t);
}

static void tb_top(textbuf *t)
{
	tb_sel_top(t);
	tb_sel_to_cursor(t);
}

static void tb_sel_bottom(textbuf *t)
{
	t->sel.c[1].y = tb_num_lines(t) - 1;
	t->sel.c[1].x = tb_cur_line_len(t);
	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_bottom(textbuf *t)
{
	tb_sel_bottom(t);
	tb_sel_to_cursor(t);
}

static void tb_copy(textbuf *t)
{
	u32 len;
	char *text = tb_sel_get(t, &len);
	clipboard_store(text);
	_free(text);
}

static void tb_cut(textbuf *t)
{
	tb_copy(t);
	tb_sel_delete(t);
	tb_scroll_to_cursor(t);
}

static void tb_paste(textbuf *t)
{
	char *text = clipboard_load();
	tb_insert(t, text);
	free(text);
	tb_scroll_to_cursor(t);
}

static void tb_scroll(textbuf *t, i32 offset)
{
	i32 y = t->page_y + offset;
	if(y < 0)
	{
		y = 0;
	}

	t->page_y = y;
}

static u32 tb_col_to_idx(textbuf *t, u32 y, u32 col)
{
	const char *line;
	u32 i, x, len;
	vector *v;

	v = tb_get_line(t, y);
	line = vector_data(v);
	len = vector_len(v);
	for(i = 0, x = 0; x < col && i < len; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return i;
}

static void tb_sel_cur_word(textbuf *t)
{
	u32 x1 = t->sel.c[1].x;
	u32 x2 = x1;
	vector *line = tb_cur_line(t);
	const char *buf = vector_data(line);
	u32 len = vector_len(line);

	if(isspace(buf[x1]))
	{
		while(x1 > 0 && isspace(buf[x1 - 1])) { --x1; }
		while(x2 < len && isspace(buf[x2])) { ++x2; }
	}
	else
	{
		while(x1 > 0 && is_ident(buf[x1 - 1])) { --x1; }
		while(x2 < len && is_ident(buf[x2])) { ++x2; }
	}

	t->sel.c[0].x = x1;
	t->sel.c[1].x = x2;
	t->cursor_save_x = -1;
}

static u32 tb_cursor_to_line(textbuf *t, u32 y)
{
	return umin(y + t->page_y, tb_num_lines(t) - 1);
}

static void tb_mouse_sel(textbuf *t, u32 x, u32 y)
{
	t->sel.c[1].y = y = tb_cursor_to_line(t, y);
	x = (x > offset_x) ? (x - offset_x) : 0;
	t->sel.c[1].x = tb_col_to_idx(t, y, x);
	t->cursor_save_x = -1;
}

static void tb_mouse_cursor(textbuf *t, u32 x, u32 y)
{
	tb_sel_to_cursor(t);
	tb_mouse_sel(t, x, y);
	tb_sel_to_cursor(t);
}

static void tb_double_click(textbuf *t, u32 x, u32 y)
{
	tb_mouse_sel(t, x, y);
	tb_sel_to_cursor(t);
	tb_sel_cur_word(t);
	(void)x;
}

static void tb_triple_click(textbuf *t, u32 x, u32 y)
{
	t->sel.c[1].y = tb_cursor_to_line(t, y);
	tb_sel_cur_line(t);
	(void)x;
}
