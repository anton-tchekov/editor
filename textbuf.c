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
	textbuf *t = _malloc(sizeof(textbuf));
	t->filename = str_heapcopy(name);
	t->language = lang;
	t->modified = 0;
	t->exists = on_disk;
	t->insert = 0;
	tb_reset_cursor(t);
	if(text)
	{
		t->lines = vec_init(count_char(text, '\n') + 1);
		char *p = text;
		char *linestart = text;
		u32 c;
		do
		{
			c = *p;
			if(c == '\0' || c == '\n')
			{
				line = vec_from(linestart, p - linestart);
				vec_push(&t->lines, sizeof(line), &line);
				linestart = p + 1;
			}
			++p;
		}
		while(c != '\0');
	}
	else
	{
		t->lines = vec_init(32 * sizeof(vec));
		line = vec_init(8);
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
	t->filename = str_heapcopy(name);
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
	for(u32 i = y1; i <= y2; ++i)
	{
		vec_destroy(tb_get_line(t, i));
	}

	vec_remove(&t->lines, y1 * sizeof(vec),
		(y2 - y1 + 1) * sizeof(vec));
}

static void tb_destroy(textbuf *t)
{
	u32 num_lines = tb_num_lines(t);
	for(u32 i = 0; i < num_lines; ++i)
	{
		vec_destroy(tb_get_line(t, i));
	}

	vec_destroy(&t->lines);
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
	selection nsel = t->sel;
	sel_norm(&nsel);

	u32 x1 = nsel.c[0].x;
	u32 y1 = nsel.c[0].y;
	u32 x2 = nsel.c[1].x;
	u32 y2 = nsel.c[1].y;

	t->sel.c[1] = t->sel.c[0] = nsel.c[0];

	vec *line = tb_get_line(t, y1);
	if(y1 == y2)
	{
		vec_remove(line, x1, x2 - x1);
	}
	else
	{
		vec *last = tb_get_line(t, y2);
		vec_replace(line, x1, vec_len(line) - x1,
			vec_str(last) + x2, vec_len(last) - x2);
		tb_remove_lines(t, y1 + 1, y2);
	}
}

static u32 tb_sel_count_bytes(textbuf *t, selection *sel)
{
	u32 x1 = sel->c[0].x;
	u32 y1 = sel->c[0].y;
	u32 x2 = sel->c[1].x;
	u32 y2 = sel->c[1].y;
	if(y1 == y2)
	{
		return x2 - x1;
	}

	u32 len = tb_line_len(t, y1) - x1 + 1;
	for(++y1; y1 < y2; ++y1)
	{
		len += tb_line_len(t, y1) + 1;
	}

	return len + x2;
}

static char *tb_sel_get(textbuf *t, u32 *out_len)
{
	selection nsel = t->sel;
	sel_norm(&nsel);

	u32 x1 = nsel.c[0].x;
	u32 y1 = nsel.c[0].y;
	u32 x2 = nsel.c[1].x;
	u32 y2 = nsel.c[1].y;

	u32 len = tb_sel_count_bytes(t, &nsel);
	*out_len = len;
	char *output = _malloc(len + 1);
	char *p = output;
	if(y1 == y2)
	{
		memcpy(p, tb_line_data(t, y1) + x1, len);
		p += len;
	}
	else
	{
		vec *line = tb_get_line(t, y1);
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
		vec *line = tb_cur_line(t);
		u32 line_len = vec_len(line);
		if(t->sel.c[1].x >= line_len)
		{
			t->sel.c[1].x = line_len;
			if(t->sel.c[1].y < tb_num_lines(t) - 1)
			{
				/* Merge with next line */
				u32 next_idx = t->sel.c[1].y + 1;
				vec *next = tb_get_line(t, next_idx);
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
		vec *line = tb_cur_line(t);
		if(t->sel.c[1].x == 0)
		{
			if(t->sel.c[1].y > 0)
			{
				/* Merge with previous line */
				vec *prev = tb_get_line(t, --t->sel.c[1].y);
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
	vec *line = tb_get_line(t, y);
	char *buf = vec_data(line);
	u32 len = vec_len(line);
	u32 i = 0;
	while(i < len && isspace(buf[i])) { ++i; }
	return i;
}

static void tb_sel_home(textbuf *t)
{
	u32 i = tb_line_start(t, t->sel.c[1].y);
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
	selection nsel = t->sel;
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
	range ri;
	if(tb_sel_y_range(t, &ri))
	{
		return;
	}

	for(u32 y = ri.a; y <= ri.b; ++y)
	{
		vec *line = tb_get_line(t, y);
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
		for(u32 y = ri.a; y <= ri.b; ++y)
		{
			vec *line = tb_get_line(t, y);
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
		tb_sel_clear(t);
		vec *line = tb_cur_line(t);
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
	vec new_line = vec_init(8);
	tb_ins_line(t, t->sel.c[1].y, &new_line);
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter_after(textbuf *t)
{
	vec new_line = vec_init(8);
	tb_ins_line(t, ++t->sel.c[1].y, &new_line);
	t->sel.c[1].x = 0;
	t->cursor_save_x = 0;
	tb_sel_to_cursor(t);
	t->modified = 1;
	tb_scroll_to_cursor(t);
}

static void tb_enter(textbuf *t)
{
	tb_sel_clear(t);

	vec *cur = tb_cur_line(t);
	char *str = vec_str(cur) + t->sel.c[1].x;
	u32 len = vec_len(cur) - t->sel.c[1].x;

	/* Copy characters after cursor on current line to new line */
	vec new_line = vec_from(str, len);

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
	t->language = inc_wrap(t->language, LANGUAGE_COUNT);
}

static u32 tb_count_bytes(textbuf *t)
{
	u32 len = 0;
	u32 num_lines = tb_num_lines(t);
	for(u32 i = 0; i < num_lines; ++i)
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
	u32 num_lines = tb_num_lines(t);
	for(u32 i = 0; i < num_lines; ++i)
	{
		vec *cur = tb_get_line(t, i);
		u32 line_len = vec_len(cur);
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
	u32 last_line = tb_num_lines(t) - 1;
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
	u32 end = tb_num_lines(t);
	for(u32 i = 0; i < end; ++i)
	{
		vec *line = tb_get_line(t, i);
		char *data = vec_data(line);
		u32 len = vec_len(line);
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

	u32 first = tb_line_len(t, t->sel.c[0].y);
	if(t->sel.c[0].x > first)
	{
		t->sel.c[0].x = first;
	}

	u32 last = tb_line_len(t, t->sel.c[1].y);
	if(t->sel.c[1].x > last)
	{
		t->cursor_save_x = -1;
		t->sel.c[1].x = last;
	}
}

static void tb_insert(textbuf *t, char *text)
{
	tb_sel_clear(t);

	/* Count number of newlines and store position of last line */
	u32 new_lines = 0;
	char *s = text;
	char *p = NULL;
	for(u32 c; (c = *s); ++s)
	{
		if(c == '\n')
		{
			p = s + 1;
			++new_lines;
		}
	}

	u32 y = t->sel.c[1].y;
	if(!new_lines)
	{
		u32 len = s - text;
		vec_insert(tb_get_line(t, y), t->sel.c[1].x, len, text);
		t->sel.c[1].x += len;
	}
	else
	{
		/* Insert new lines in advance to avoid quadratic complexity */
		tb_ins_lines(t, y + 1, new_lines);
		vec *lines = vec_data(&t->lines);

		/* Last line */
		vec *first = tb_get_line(t, y);
		u32 rlen = vec_len(first) - t->sel.c[1].x;
		u32 slen = s - p;

		vec line = vec_init(rlen + slen);
		vec_push(&line, slen, p);
		vec_push(&line, rlen, vec_str(first) + t->sel.c[1].x);
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
			lines[++y] = vec_from(text, p - text);
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

static void tb_goto_def(textbuf *t, char *s)
{
	u32 len = tb_num_lines(t);
	u32 sl = strlen(s);
	for(u32 i = 0; i < len; ++i)
	{
		vec *line = tb_get_line(t, i);
		u32 ll = vec_len(line);
		char *buf = vec_data(line);
		if(ll == 0 || isspace(buf[0]))
		{
			continue;
		}

		i32 offset = str_find(buf, ll, s, sl);
		if(offset < 0)
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
	u32 x = 0;
	char *line = tb_line_data(t, y);
	for(u32 i = 0; i < end; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return x;
}

static void tb_move_vertical(textbuf *t, u32 prev_y)
{
	vec *line = tb_cur_line(t);
	u32 len = vec_len(line);
	char *buf = vec_data(line);
	if(t->cursor_save_x < 0)
	{
		t->cursor_save_x = tb_cursor_pos_x(t, prev_y, t->sel.c[1].x);
	}

	u32 max_x = t->cursor_save_x;
	u32 i = 0;
	for(u32 x = 0; i < len && x < max_x; ++i)
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
			char *buf = tb_cur_line_data(t);
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
			char *buf = tb_cur_line_data(t);
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
	char *utf8 = convert_to_utf8(text, &len);
	_free(text);
	clipboard_store(utf8);
	_free(utf8);
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
	u32 len = strlen(text);
	char *conv = convert_from_utf8(text, &len);
	free(text);
	if(!conv)
	{
		msg_show(MSG_ERROR, "Unsupported UTF8 character");
		return;
	}

	tb_insert(t, conv);
	_free(conv);
	tb_scroll_to_cursor(t);
}

static void tb_scroll(textbuf *t, i32 offset)
{
	i32 y = t->page_y + offset;
	if(y < 0)
	{
		y = 0;
	}

	u32 num_lines = tb_num_lines(t);
	if(y + _screen_height > num_lines)
	{
		y = (num_lines > _screen_height) ? num_lines - _screen_height : 0;
	}

	t->page_y = y;
}

static u32 tb_col_to_idx(textbuf *t, u32 y, u32 col)
{
	vec *v = tb_get_line(t, y);
	char *line = vec_data(v);
	u32 len = vec_len(v);
	u32 i = 0;
	for(u32 x = 0; x < col && i < len; ++i)
	{
		x = tb_chr_x_inc(line[i], x);
	}

	return i;
}

static void tb_sel_cur_word(textbuf *t)
{
	u32 x1 = t->sel.c[1].x;
	u32 x2 = x1;
	vec *line = tb_cur_line(t);
	u32 len = vec_len(line);
	char *buf = vec_data(line);
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
	vec *line = tb_get_line(t, y);
	for(u32 qc; (qc = *q); ++q)
	{
		u32 tc;
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
	cursor *last = sel_last(&t->sel);
	u32 x = last->x;
	u32 y = last->y;
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
	u32 i = sizeof(define) - 1;
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
	if(i >= len) { return 0; }

	u32 nlen;
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
	/* Determine identifier length */
	u32 slen = i;
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
	char *s = vec_str(v);
	u32 len = vec_len(v);
	u32 i = tb_ad_is_define(s, len);
	if(!i) { return; }
	u32 slen = tb_ad_identlen(s, i, len);
	if(!slen) { return; }
	i += slen;
	*maxs = umax(slen, *maxs);
	i = tb_ad_skipspace(s, i, len);
	u32 nlen = tb_ad_declen(s, i, len);
	if(!nlen) { return; }
	*maxn = umax(nlen, *maxn);
}

static u32 revspace(char *s, u32 i, u32 len)
{
	for(; len > i && isspace(s[len - 1]); --len) {}
	return len;
}

static void tb_ad_line_mod(vec *v, u32 pad)
{
	char *s = vec_str(v);
	u32 len = vec_len(v);
	u32 i = tb_ad_is_define(s, len);
	if(!i) { return; }
	u32 spos = i;
	u32 slen = tb_ad_identlen(s, i, len);
	if(!slen) { return; }
	i += slen;
	u32 total = tb_ad_skipspace(s, i, len);
	if(total == len)
	{
		/* If nothing after identifier, trim trailing spaces */
		v->len = i;
		return;
	}

	i = total;
	u32 npos = i;
	total = sizeof(define) - 1;

	/* Check if decimal number */
	u32 nlen = tb_ad_declen(s, i, len);
	u32 nspc;
	if(nlen)
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
	vec repl = vec_init(total);
	vec_push(&repl, sizeof(define) - 1, define);
	vec_push(&repl, slen, s + spos);
	vec_pushn(&repl, ' ', nspc);
	vec_push(&repl, nlen, s + npos);
	vec_destroy(v);
	*v = repl;
}

static void tb_align_defines(textbuf *t)
{
	range r;
	if(tb_sel_y_range(t, &r))
	{
		msg_show(MSG_INFO, "No selection!");
		return;
	}

	u32 maxs = 0;
	u32 maxn = 0;

	/* Get length of longest identifier and longest decimal integer */
	for(u32 y = r.a; y <= r.b; ++y)
	{
		tb_ad_line_stat(tb_get_line(t, y), &maxs, &maxn);
	}

	if(!maxs)
	{
		msg_show(MSG_INFO, "No #define in selection!");
		return;
	}

	/* Process each line and align parts */
	for(u32 y = r.a; y <= r.b; ++y)
	{
		tb_ad_line_mod(tb_get_line(t, y), maxs + maxn + 1);
	}

	msg_show(MSG_INFO, "Aligned defines (MaxS = %d, MaxN = %d)", maxs, maxn);
}

static char *tb_cur_line_span(textbuf *t, u32 *out_len)
{
	selection nsel = t->sel;
	sel_norm(&nsel);
	if(nsel.c[0].y == nsel.c[1].y)
	{
		/* Selection has to be on a single line */
		if(nsel.c[0].x == nsel.c[1].x)
		{
			/* Return current word */
			u32 x = nsel.c[0].x;
			vec *v = tb_get_line(t, nsel.c[0].y);
			char *s = vec_str(v);
			u32 len = vec_len(v);

			/* Find start */
			u32 start = x;
			for(; start > 0 && is_ident(s[start - 1]); --start) {}

			/* Find end */
			u32 end = x;
			for(; end < len && is_ident(s[end]); ++end) {}

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
