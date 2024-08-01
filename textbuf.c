typedef struct
{
	vec lines;
	selection sel;
	char *filename;
	u32 page_y;
	i32 cursor_save_x;
	u8 language;
	u8 modified;
	u8 exists;
	u8 insert;
} textbuf;

static u32 tb_num_lines(textbuf *t)
{
	return vec_len(&t->lines) / sizeof(vec);
}

static void tb_reset_cursor(textbuf *t)
{
	t->page_y = 0;
	t->cursor_save_x = 0;
	memset(&t->sel, 0, sizeof(t->sel));
}

static textbuf *tb_new(char *name, char *text, u32 on_disk, u32 lang)
{
	vec line;
	textbuf *t;

	t = _malloc(sizeof(textbuf));
	t->filename = _strdup(name);
	t->language = lang;
	t->modified = 0;
	t->exists = on_disk;
	t->insert = 0;
	tb_reset_cursor(t);
	if(text)
	{
		u32 c;
		char *linestart, *p;

		vec_init(&t->lines, count_char(text, '\n') + 1);
		p = text;
		linestart = text;
		do
		{
			c = *p;
			if(c == '\0' || c == '\n')
			{
				vec_from(&line, linestart, p - linestart);
				vec_push(&t->lines, sizeof(line), &line);
				linestart = p + 1;
			}
			++p;
		}
		while(c != '\0');
	}
	else
	{
		vec_init(&t->lines, 32 * sizeof(vec));
		vec_init(&line, 8);
		vec_push(&t->lines, sizeof(line), &line);
	}

	return t;
}

static void tb_toggle_insert(textbuf *t)
{
	t->insert = !t->insert;
}

static void tb_change_filename(textbuf *t, char *name)
{
	_free(t->filename);
	t->filename = _strdup(name);
}

static vec *tb_get_line(textbuf *t, u32 i)
{
	return (vec *)vec_get(&t->lines, i * sizeof(vec));
}

static vec *tb_cur_line(textbuf *t)
{
	return tb_get_line(t, t->sel.c[1].y);
}

static u32 tb_line_len(textbuf *t, u32 i)
{
	return vec_len(tb_get_line(t, i));
}

static char *tb_line_data(textbuf *t, u32 i)
{
	return vec_data(tb_get_line(t, i));
}

static u32 tb_cur_line_len(textbuf *t)
{
	return vec_len(tb_cur_line(t));
}

static char *tb_cur_line_data(textbuf *t)
{
	return vec_data(tb_cur_line(t));
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

static void tb_ins_line(textbuf *t, u32 line, vec *v)
{
	vec_insert(&t->lines, line * sizeof(vec), sizeof(vec), v);
}

static void tb_ins_lines(textbuf *t, u32 y, u32 count)
{
	vec_makespace(&t->lines, y * sizeof(vec), count * sizeof(vec));
}

static void tb_remove_line(textbuf *t, u32 line)
{
	vec_destroy(tb_get_line(t, line));
	vec_remove(&t->lines, line * sizeof(vec), sizeof(vec));
}

static void tb_remove_lines(textbuf *t, u32 y1, u32 y2)
{
	u32 i;

	for(i = y1; i <= y2; ++i)
	{
		vec_destroy(tb_get_line(t, i));
	}

	vec_remove(&t->lines, y1 * sizeof(vec),
		(y2 - y1 + 1) * sizeof(vec));
}

static void tb_destroy(textbuf *t)
{
	u32 i, num_lines;

	num_lines = tb_num_lines(t);
	for(i = 0; i < num_lines; ++i)
	{
		vec_destroy(tb_get_line(t, i));
	}

	vec_destroy(&t->lines);
	_free(t->filename);
	_free(t);
}

static void tb_sel_all(textbuf *t)
{
	u32 last_line;

	last_line = tb_num_lines(t) - 1;
	t->sel.c[0].x = 0;
	t->sel.c[0].y = 0;
	t->sel.c[1].x = tb_line_len(t, last_line);
	t->sel.c[1].y = last_line;
}

static void tb_sel_delete(textbuf *t)
{
	u32 x1, y1, x2, y2;
	vec *line;
	selection nsel;

	nsel = t->sel;
	sel_norm(&nsel);

	x1 = nsel.c[0].x;
	y1 = nsel.c[0].y;
	x2 = nsel.c[1].x;
	y2 = nsel.c[1].y;

	t->sel.c[1] = t->sel.c[0] = nsel.c[0];

	line = tb_get_line(t, y1);
	if(y1 == y2)
	{
		vec_remove(line, x1, x2 - x1);
	}
	else
	{
		vec *last;

		last = tb_get_line(t, y2);
		vec_replace(line, x1, vec_len(line) - x1,
			vec_str(last) + x2, vec_len(last) - x2);
		tb_remove_lines(t, y1 + 1, y2);
	}
}

static u32 tb_sel_count_bytes(textbuf *t, selection *sel)
{
	u32 len, x1, y1, x2, y2;

	x1 = sel->c[0].x;
	y1 = sel->c[0].y;
	x2 = sel->c[1].x;
	y2 = sel->c[1].y;
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
	selection nsel;

	nsel = t->sel;
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
		vec *line;

		line = tb_get_line(t, y1);
		len = vec_len(line) - x1;
		memcpy(p, (u8 *)vec_data(line) + x1, len);
		p += len;
		for(++y1; y1 < y2; ++y1)
		{
			*p++ = '\n';
			line = tb_get_line(t, y1);
			len = vec_len(line);
			memcpy(p, vec_data(line), len);
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
		vec *line;
		u32 line_len;

		line = tb_cur_line(t);
		line_len = vec_len(line);
		if(t->sel.c[1].x >= line_len)
		{
			t->sel.c[1].x = line_len;
			if(t->sel.c[1].y < tb_num_lines(t) - 1)
			{
				/* Merge with next line */
				u32 next_idx;
				vec *next;

				next_idx = t->sel.c[1].y + 1;
				next = tb_get_line(t, next_idx);
				vec_push(line, vec_len(next), vec_data(next));
				tb_remove_line(t, next_idx);
			}
		}
		else
		{
			/* Delete next char */
			vec_remove(line, t->sel.c[1].x, 1);
		}

		t->cursor_save_x = -1;
	}

	t->modified = 1;
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
		vec *line;

		line = tb_cur_line(t);
		if(t->sel.c[1].x == 0)
		{
			if(t->sel.c[1].y > 0)
			{
				/* Merge with previous line */
				vec *prev;

				prev = tb_get_line(t, --t->sel.c[1].y);
				t->sel.c[1].x = vec_len(prev);
				vec_push(prev, vec_len(line), vec_data(line));
				tb_remove_line(t, t->sel.c[1].y + 1);
			}
		}
		else
		{
			/* Delete previous char */
			vec_remove(line, --t->sel.c[1].x, 1);
		}

		t->cursor_save_x = -1;
		tb_sel_to_cursor(t);
	}

	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static u32 tb_line_start(textbuf *t, u32 y)
{
	vec *line;
	char *buf;
	u32 i, len;

	line = tb_get_line(t, y);
	buf = vec_data(line);
	len = vec_len(line);
	i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	return i;
}

static void tb_sel_home(textbuf *t)
{
	u32 i;

	i = tb_line_start(t, t->sel.c[1].y);
	t->sel.c[1].x = (t->sel.c[1].x == i) ? 0 : i;
	t->cursor_save_x = -1;
	tb_scroll_to_cursor(t);
}

static void tb_home(textbuf *t)
{
	tb_sel_home(t);
	tb_sel_to_cursor(t);
}

typedef struct
{
	u32 a, b;
} range;

static u32 tb_sel_y_range(textbuf *t, range *r)
{
	selection nsel;

	nsel = t->sel;
	sel_norm(&nsel);
	if(nsel.c[0].y == nsel.c[1].y)
	{
		if(nsel.c[0].x == 0 && nsel.c[1].x != 0 &&
			nsel.c[1].x == tb_line_len(t, nsel.c[0].y))
		{
			r->b = r->a = nsel.c[0].y;
			return 0;
		}

		return 1;
	}

	r->a = nsel.c[0].y;
	r->b = (nsel.c[1].x > 0) ? nsel.c[1].y : (nsel.c[1].y - 1);
	return 0;
}

static void tb_fix_sel_unindent(textbuf *t, cursor *c)
{
	if(c->x >= tb_line_start(t, c->y) && c->x > 0)
	{
		--c->x;
	}
}

static void tb_sel_unindent(textbuf *t)
{
	vec *line;
	u32 y1, y2;
	range ri;

	if(tb_sel_y_range(t, &ri))
	{
		return;
	}

	y1 = ri.a;
	y2 = ri.b;
	for(; y1 <= y2; ++y1)
	{
		line = tb_get_line(t, y1);
		if(vec_len(line) > 0 && vec_str(line)[0] == '\t')
		{
			vec_remove(line, 0, 1);
		}
	}

	tb_fix_sel_unindent(t, &t->sel.c[0]);
	tb_fix_sel_unindent(t, &t->sel.c[1]);
	t->cursor_save_x = -1;
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_fix_sel_indent(textbuf *t, cursor *c)
{
	if(c->x > tb_line_start(t, c->y))
	{
		++c->x;
	}
}

static void tb_char(textbuf *t, u32 c)
{
	range ri;

	if(c == '\t' && !tb_sel_y_range(t, &ri))
	{
		/* Indent selection */
		u32 y1, y2;

		y1 = ri.a;
		y2 = ri.b;
		for(; y1 <= y2; ++y1)
		{
			vec *line;

			line = tb_get_line(t, y1);
			if(vec_len(line) > 0)
			{
				vec_insert(line, 0, 1, "\t");
			}
		}

		tb_fix_sel_indent(t, &t->sel.c[0]);
		tb_fix_sel_indent(t, &t->sel.c[1]);
	}
	else
	{
		vec *line;

		tb_sel_clear(t);
		line = tb_cur_line(t);
		if(t->insert && t->sel.c[1].x < vec_len(line))
		{
			vec_str(line)[t->sel.c[1].x] = c;
		}
		else
		{
			u8 ins[1];

			ins[0] = c;
			vec_insert(line, t->sel.c[1].x, 1, ins);
		}

		++t->sel.c[1].x;
		tb_sel_to_cursor(t);
	}

	t->cursor_save_x = -1;
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter_before(textbuf *t)
{
	vec new_line;

	vec_init(&new_line, 8);
	tb_ins_line(t, t->sel.c[1].y, &new_line);
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter_after(textbuf *t)
{
	vec new_line;

	vec_init(&new_line, 8);
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
	vec new_line, *cur;

	tb_sel_clear(t);

	cur = tb_cur_line(t);
	str = vec_str(cur) + t->sel.c[1].x;
	len = vec_len(cur) - t->sel.c[1].x;

	/* Copy characters after cursor on current line to new line */
	vec_init(&new_line, len);
	vec_push(&new_line, len, str);

	/* Insert new line */
	tb_ins_line(t, t->sel.c[1].y + 1, &new_line);

	/* Remove characters after cursor on current line */
	vec_remove(tb_cur_line(t), t->sel.c[1].x, len);

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
	u32 count;

	count = tb_num_lines(t);
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
	t->language = inc_wrap(t->language, LANGUAGE_COUNT);
}

static u32 tb_count_bytes(textbuf *t)
{
	u32 i, len, num_lines;

	num_lines = tb_num_lines(t);
	for(i = 0, len = 0; i < num_lines; ++i)
	{
		len += tb_line_len(t, i) + 1;
	}

	return len - 1;
}

static char *tb_export(textbuf *t, u32 *len)
{
	u32 i, num_lines, bytes;
	char *p, *buf;

	bytes = tb_count_bytes(t);
	p = buf = _malloc(bytes);
	num_lines = tb_num_lines(t);
	for(i = 0; i < num_lines; ++i)
	{
		vec *cur;
		u32 line_len;

		cur = tb_get_line(t, i);
		line_len = vec_len(cur);
		memcpy(p, vec_data(cur), line_len);
		p += line_len;
		if(i < num_lines - 1)
		{
			*p++ = '\n';
		}
	}

	*len = bytes;
	return buf;
}

static void tb_ins(textbuf *t, char *s, u32 len, u32 inc)
{
	tb_sel_clear(t);
	vec_insert(tb_cur_line(t), t->sel.c[1].x, len, s);
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
	u32 last_line;

	last_line = tb_num_lines(t) - 1;
	t->sel.c[0].x = 0;
	if(t->sel.c[1].y < last_line)
	{
		t->sel.c[1].x = 0;
		++t->sel.c[1].y;
	}
	else
	{
		t->sel.c[1].x = tb_line_len(t, last_line);
	}

	tb_scroll_to_cursor(t);
}

static void tb_trailing(textbuf *t)
{
	u32 i, len, end;
	vec *line;
	char *data;

	end = tb_num_lines(t);
	for(i = 0; i < end; ++i)
	{
		line = tb_get_line(t, i);
		data = vec_data(line);
		len = vec_len(line);
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

static void tb_insert(textbuf *t, char *text)
{
	u32 c, y, new_lines;
	char *p, *s;

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
		u32 len;

		len = s - text;
		vec_insert(tb_get_line(t, y), t->sel.c[1].x, len, text);
		t->sel.c[1].x += len;
	}
	else
	{
		char *last;
		u32 slen, rlen;
		vec line, *first, *lines;

		/* Insert new lines in advance to avoid quadratic complexity */
		tb_ins_lines(t, y + 1, new_lines);
		lines = vec_data(&t->lines);

		/* Last line */
		first = tb_get_line(t, y);
		rlen = vec_len(first) - t->sel.c[1].x;
		slen = s - p;
		vec_init_full(&line, rlen + slen);
		last = vec_data(&line);
		memcpy(last, p, slen);
		memcpy(last + slen, (u8 *)vec_data(first) + t->sel.c[1].x, rlen);
		lines[y + new_lines] = line;

		first->len = t->sel.c[1].x;
		t->sel.c[1].x = slen;
		t->sel.c[1].y = y + new_lines;

		/* First line */
		p = strchr(text, '\n');
		vec_push(tb_get_line(t, y), p - text, text);
		text = p + 1;

		/* Other lines */
		while((p = strchr(text, '\n')))
		{
			vec_from(&line, text, p - text);
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

static void tb_goto_xy(textbuf *t, u32 x, u32 y)
{
	u32 num_lines;

	num_lines = tb_num_lines(t);
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

static void tb_goto_def(textbuf *t, char *s)
{
	u32 i, len, sl, ll;
	vec *line;
	char *buf;
	i32 offset;

	len = tb_num_lines(t);
	sl = strlen(s);
	for(i = 0; i < len; ++i)
	{
		line = tb_get_line(t, i);
		ll = vec_len(line);
		buf = vec_data(line);
		if(ll == 0 || isspace(buf[0]))
		{
			continue;
		}

		if((offset = str_find(buf, ll, s, sl)) < 0)
		{
			continue;
		}

		tb_goto_xy(t, offset, i);
		return;
	}
}

static u32 tb_chr_x_inc(u32 c, u32 x)
{
	return (c == '\t') ?
		((x + _tabsize) / _tabsize * _tabsize) :
		(x + 1);
}

static u32 tb_cursor_pos_x(textbuf *t, u32 y, u32 end)
{
	u32 i, x;
	char *line;

	line = tb_line_data(t, y);
	for(i = 0, x = 0; i < end; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return x;
}

static void tb_move_vertical(textbuf *t, u32 prev_y)
{
	u32 i, x, max_x, len;
	vec *line;
	char *buf;

	line = tb_cur_line(t);
	len = vec_len(line);
	buf = vec_data(line);
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
		u32 prev_y;

		prev_y = t->sel.c[1].y;
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
		u32 prev_y;

		prev_y = t->sel.c[1].y;
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
		u32 prev_y;

		prev_y = t->sel.c[1].y;
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
	u32 num_lines, prev_y;

	num_lines = tb_num_lines(t);
	prev_y = t->sel.c[1].y;
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
			char *buf;
			u32 i, type;

			buf = tb_cur_line_data(t);
			i = t->sel.c[1].x - 1;
			type = char_type(buf[i]);
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
	u32 len;

	len = tb_cur_line_len(t);
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
			char *buf;
			u32 i, type;

			buf = tb_cur_line_data(t);
			i = t->sel.c[1].x;
			type = char_type(buf[i]);
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
	char *text;

	text = tb_sel_get(t, &len);
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
	char *text;

	text = clipboard_load();
	tb_insert(t, text);
	free(text);
	tb_scroll_to_cursor(t);
}

static void tb_scroll(textbuf *t, i32 offset)
{
	u32 num_lines;
	i32 y;

	y = t->page_y + offset;
	if(y < 0)
	{
		y = 0;
	}

	num_lines = tb_num_lines(t);
	if(y + _screen_height > num_lines)
	{
		y = (num_lines > _screen_height) ? num_lines - _screen_height : 0;
	}

	t->page_y = y;
}

static u32 tb_col_to_idx(textbuf *t, u32 y, u32 col)
{
	char *line;
	u32 i, x, len;
	vec *v;

	v = tb_get_line(t, y);
	line = vec_data(v);
	len = vec_len(v);
	for(i = 0, x = 0; x < col && i < len; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return i;
}

static void tb_sel_cur_word(textbuf *t)
{
	u32 x1, x2, len;
	vec *line;
	char *buf;

	x1 = t->sel.c[1].x;
	x2 = x1;
	line = tb_cur_line(t);
	len = vec_len(line);
	buf = vec_data(line);

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
	x = (x > _offset_x) ? (x - _offset_x) : 0;
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

/* TODO: Unfinished */
static u32 tb_matches(textbuf *t, u32 x, u32 y, char *q)
{
	u32 qc, tc;
	vec *line;

	line = tb_get_line(t, y);
	for(; (qc = *q); ++q)
	{
		if(x >= vec_len(line))
		{
			tc = '\n';
			if(y >= tb_num_lines(t))
			{
				/* Restart search from the beginning of file */
				return 0;
			}

			x = 0;
			++y;
			line = tb_get_line(t, y);
		}
		else
		{
			tc = vec_str(line)[x];
		}

		if(qc != tc)
		{
			/* No match */
			return 0;
		}
	}

	return 1;
}

static void tb_find_next(textbuf *t, char *q)
{
	u32 x, y;
	cursor *last;

	last = sel_last(&t->sel);
	x = last->x;
	y = last->y;

	for(;;)
	{
		if(tb_matches(t, x, y, q))
		{
			return;
		}
	}
}

static char define[] = "#define ";

static u32 tb_ad_is_define(char *s, u32 len)
{
	u32 i;

	i = sizeof(define) - 1;
	if((i < len) && !memcmp(s, define, i))
	{
		/* Skip spaces after #define */
		for(; i < len && isspace(s[i]); ++i) {}

		/* Check for valid identifier */
		if(i >= len || !is_ident_start(s[i]))
		{
			return 0;
		}

		return i;
	}

	return 0;
}

static u32 tb_ad_declen(char *s, u32 i, u32 len)
{
	u32 nlen;

	if(i >= len) { return 0; }

	if(s[i] == '0')
	{
		++i;
		nlen = 1;
	}
	else if(is_1_to_9(s[i]))
	{
		/* Determine number length */
		nlen = i;
		for(++i; i < len && isdigit(s[i]); ++i) {}
		nlen = i - nlen;
	}
	else
	{
		return 0;
	}

	/* Check that there are only spaces after the number */
	for(; i < len; ++i)
	{
		if(!isspace(s[i]))
		{
			return 0;
		}
	}

	return nlen;
}

static u32 tb_ad_identlen(char *s, u32 i, u32 len)
{
	u32 slen;

	/* Determine identifier length */
	slen = i;
	for(++i; i < len && is_ident(s[i]); ++i) {}
	slen = i - slen;

	/* Skip macro if it has parameters */
	if(i < len && s[i] == '(')
	{
		return 0;
	}

	return slen;
}

static u32 tb_ad_skipspace(char *s, u32 i, u32 len)
{
	for(; i < len && isspace(s[i]); ++i) {}
	return i;
}

static void tb_ad_line_stat(vec *v, u32 *maxs, u32 *maxn)
{
	u32 i, len, slen, nlen;
	char *s;

	s = vec_str(v);
	len = vec_len(v);
	if(!(i = tb_ad_is_define(s, len))) { return; }
	if(!(slen = tb_ad_identlen(s, i, len))) { return; }
	i += slen;
	*maxs = umax(slen, *maxs);
	i = tb_ad_skipspace(s, i, len);
	if(!(nlen = tb_ad_declen(s, i, len))) { return; }
	*maxn = umax(nlen, *maxn);
}

static char *padchr(char *s, u32 c, u32 count)
{
	while(count)
	{
		--count;
		*s++ = c;
	}

	return s;
}

static char *append(char *dst, char *src, size_t count)
{
	memcpy(dst, src, count);
	return dst + count;
}

static u32 revspace(char *s, u32 i, u32 len)
{
	for(; len > i && isspace(s[len - 1]); --len) {}
	return len;
}

static void tb_ad_line_mod(vec *v, u32 pad)
{
	vec repl;
	u32 i, len, spos, slen, npos, nlen, nspc, total;
	char *s, *rs;

	s = vec_str(v);
	len = vec_len(v);
	if(!(i = tb_ad_is_define(s, len))) { return; }
	spos = i;
	if(!(slen = tb_ad_identlen(s, i, len))) { return; }
	i += slen;
	if((total = tb_ad_skipspace(s, i, len)) == len)
	{
		/* If nothing after identifier, trim trailing spaces */
		v->len = i;
		return;
	}

	i = total;
	npos = i;
	total = sizeof(define) - 1;

	/* Check if decimal number */
	if((nlen = tb_ad_declen(s, i, len)))
	{
		total += pad;
		nspc = pad - slen - nlen;
	}
	else
	{
		/* Scan reverse to find definition end */
		nlen = revspace(s, i, len) - npos;
		total += pad + nlen;
		nspc = pad - slen;
	}

	/* Rebuild vec */
	vec_init_full(&repl, total);
	rs = vec_str(&repl);
	rs = append(rs, define, sizeof(define) - 1);
	rs = append(rs, s + spos, slen);
	rs = padchr(rs, ' ', nspc);
	append(rs, s + npos, nlen);
	vec_destroy(v);
	*v = repl;
}

static void tb_align_defines(textbuf *t)
{
	u32 y, end, maxs, maxn;
	range r;

	if(tb_sel_y_range(t, &r))
	{
		msg_show(MSG_INFO, "No selection!");
		return;
	}

	end = r.b;
	maxs = 0;
	maxn = 0;

	/* Get length of longest identifier and longest decimal integer */
	for(y = r.a; y <= end; ++y)
	{
		tb_ad_line_stat(tb_get_line(t, y), &maxs, &maxn);
	}

	if(!maxs)
	{
		msg_show(MSG_INFO, "No #define in selection!");
		return;
	}

	/* Process each line and align parts */
	for(y = r.a; y <= end; ++y)
	{
		tb_ad_line_mod(tb_get_line(t, y), maxs + maxn + 1);
	}

	msg_show(MSG_INFO, "Aligned defines (MaxS = %d, MaxN = %d)", maxs, maxn);
}

static char *tb_cur_line_span(textbuf *t, u32 *out_len)
{
	selection nsel;

	nsel = t->sel;
	sel_norm(&nsel);
	if(nsel.c[0].y == nsel.c[1].y)
	{
		/* Selection has to be on a single line */
		if(nsel.c[0].x == nsel.c[1].x)
		{
			/* Return current word */
			vec *v;
			char *s;
			u32 start, end, x, len;

			x = nsel.c[0].x;
			v = tb_get_line(t, nsel.c[0].y);
			s = vec_str(v);
			len = vec_len(v);

			/* Find start */
			for(start = x; start > 0 && is_ident(s[start - 1]); --start) {}

			/* Find end */
			for(end = x; end < len && is_ident(s[end]); ++end) {}

			if(start == end)
			{
				return NULL;
			}
			else
			{
				*out_len = end - start;
				return s + start;
			}
		}
		else
		{
			/* Return selection */
			*out_len = nsel.c[1].x - nsel.c[0].x;
			return tb_line_data(t, nsel.c[0].y) + nsel.c[0].x;
		}
	}

	return NULL;
}
