typedef struct
{
	size_t X, Y;
} Cursor;

typedef struct
{
	Cursor Limits[2];
} Selection;

typedef struct
{
	size_t Length, Capacity;
	char Buffer[];
} TextLine;

typedef struct
{
	size_t Col;
	Selection Sel;
	size_t Count, Capacity;
	TextLine **Lines;
} TextBuffer;

#if 0

static const char *textbuffer_get_line(TextBuffer *tb,
	size_t y, size_t *len)
{
	TextLine *line;

	assert(tb);
	assert(y < tb->Count);
	assert(len);

	line = tb->Lines[y];
	*len = line->Length;
	return line->Buffer;
}

static size_t textbuffer_line_length(TextBuffer *tb, size_t y)
{
	assert(tb);
	assert(y < tb->Count);
	return tb->Lines[y]->Length;
}

static size_t textbuffer_lines_count(TextBuffer *tb)
{
	assert(tb);
	return tb->Count;
}

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

static size_t double_until(size_t v, size_t limit)
{
	while(v < limit)
	{
		v <<= 1;
	}

	return v;
}

static TextLine *textline_require(TextLine *tl, size_t capacity)
{
	if(tl->Capacity < capacity)
	{
		size_t new_size = double_until(tl->Capacity, capacity);
		tl = _realloc(tl, sizeof(*tl) + new_size);
		tl->Capacity = new_size;
	}

	return tl;
}

static TextLine *textline_create(const char *content, size_t len)
{
	TextLine *tl = _malloc(sizeof(*tl) + len);
	tl->Capacity = len;
	tl->Length = len;
	memcpy(tl->Buffer, content, len);
	return tl;
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

static TextLine *cur_line(TextBuffer *tb)
{
	return tb->Lines[tb->Sel.Limits[0].Y];
}

static size_t cur_line_len(TextBuffer *tb)
{
	return cur_line(tb)->Length;
}

static void cursor_left(TextBuffer *tb)
{
	size_t *x = &tb->Sel.Limits[0].X;
	size_t *y = &tb->Sel.Limits[0].Y;
	if(*x > 0)
	{
		--(*x);
	}
	else
	{
		if(*y > 0)
		{
			--(*y);
			*x = 0;
		}
	}
}

static void cursor_right(TextBuffer *tb)
{
	size_t *x = &tb->Sel.Limits[0].X;
	size_t *y = &tb->Sel.Limits[0].Y;
	if(*x < cur_line_len(tb))
	{
		++(*x);
	}
	else
	{
		if(*y < tb->Count)
		{
			++(*y);
			*x = 0;
		}
	}
}

static void cursor_end(TextBuffer *tb)
{
	tb->Sel.Limits[0].X = cur_line_len(tb);
}

static void cursor_home(TextBuffer *tb)
{
	size_t *x = &tb->Sel.Limits[0].X;
	size_t i = 0;
	if(*x == 0)
	{
		TextLine *line = cur_line(tb);
		const char *buf = line->Buffer;
		size_t len = line->Length;
		for(; i < len && isspace(buf[i]); ++i) {}
	}

	*x = i;
}

static void cursor_up(TextBuffer *tb)
{

}

static void cursor_down(TextBuffer *tb)
{

}

static void cursor_ctrl_uo(TextBuffer *tb)
{

}

static void ctrl_down(TextBuffer *tb)
{

}

static void textbuffer_delete(TextBuffer *tb)
{

}

static void textbuffer_insert(TextBuffer *tb)
{

}

static TextLine *textline_replace(TextLine *tl, const char *text, size_t len,
	size_t x1, size_t x2)
{
	size_t diff = x2 - x1;
	size_t new_len = tl->Length - diff + len;
	tl = textline_require(tl, new_len);
	memmove(tl->Buffer + x1 + diff, tl->Buffer + x2, tl->Length - x2);
	memcpy(tl->Buffer + x1, text, len);
	tl->Length = new_len;
	return tl;
}

static void textbuffer_require(TextBuffer *tb, size_t capacity)
{
	if(tb->Capacity < capacity)
	{
		size_t new_size = double_until(tb->Capacity, capacity);
		tb->Lines = _realloc(tb->Lines, new_size * sizeof(*tb->Lines));
		tb->Capacity = capacity;
	}
}

static void textbuffer_init(TextBuffer *tb)
{
	tb->Capacity = 3;
	tb->Count = 3;
	tb->Lines = _malloc(tb->Capacity * sizeof(*tb->Lines));
	tb->Lines[0] = textline_create("Hello world", 11);
	tb->Lines[1] = textline_create("This is a test", 14);
	tb->Lines[2] = textline_create("Third Line", 10);
}

static TextBuffer *textbuffer_create(void)
{
	TextBuffer *tb = _malloc(sizeof(*tb));
	textbuffer_init(tb);
	return tb;
}

static void textbuffer_print(TextBuffer *tb)
{
	size_t i;
	for(i = 0; i < tb->Count; ++i)
	{
		printf("%5d | %.*s\n", (int)(i + 1), (int)(tb->Lines[i]->Length),
			tb->Lines[i]->Buffer);
	}

	printf("\nPrinted %d lines total.\n", (int)(tb->Count));
}

static void textbuffer_destroy(TextBuffer *tb)
{
	TextLine **start = tb->Lines;
	TextLine **end = start + tb->Count;
	while(start < end)
	{
		free(*start);
		++start;
	}

	free(tb->Lines);
	free(tb);
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

static void textbuffer_replace(TextBuffer *tb, const Selection *sel,
	const char *text, size_t len)
{
	Selection nsel = *sel;
	size_t ins_lines;
	size_t x1, y1, x2, y2;
	TextLine **lines;

	x1 = nsel.Limits[0].X;
	y1 = nsel.Limits[0].Y;
	x2 = nsel.Limits[1].X;
	y2 = nsel.Limits[1].Y;

	selection_normalize(tb, &nsel);

	ins_lines = count_char(text, len, '\n');

	textbuffer_insert_lines(tb, y1, y2, ins_lines);

/*
	const char *p = text;
	const char *end = text + len;
	int i;
	size_t idx = y1;
	lines = tb->Lines;
	for(i = 0; i <= ins_lines; ++i, ++idx)
	{
		p = memchr(text, '\n', len);
		if(idx > y2)
		{
			lines[idx] = textline_create("", 0);
		}

		if(idx == y1 && idx == y2)
		{
			lines[idx] = textline_replace();
		}
		else if(idx == y1)
		{
		}
		else if(idx == y2)
		{

		}
		else
		{

		}

		printf("%d -> %.*s\n", i, p - text, text);
		++p;
		len = end - p;
		text = p;
	}

	lines = tb->Lines;
	cur = lines + y1;
	end = lines + y2;

	while(cur < end)
	{
		tb->Lines[y2] = textline_create(line, line_len);
	}

	if(ins_lines == 1)
	{
		textline_replace(tl, text, len, x1, );
	}
	else
	{

	}

	while()
	{
		const char *p = memchr(text, len, '\n');
	}
*/
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

	assert(y1 < tb->Count);
	assert(y2 < tb->Count);

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

void test_textbuffer(void)
{
	char *copy = NULL;
	size_t len = 0;

	Selection test = { { { 9, 2 }, { 6, 0 } } };

	TextBuffer *tb = textbuffer_create();

	copy = textbuffer_get(tb, &test, &len);

	printf("Length of selection: %d\n", len);
	printf("Copy of selection: %s\n", copy);

	free(copy);

	textbuffer_replace(tb, &test, "a\nq\nqq\naaa", 10);

	textbuffer_destroy(tb);
}

#endif
