typedef struct CURSOR
{
	size_t X;
	size_t Y;
} Cursor;

typedef struct SELECTION
{
	Cursor Limits[2];
} Selection;

typedef struct TEXTLINE
{
	size_t Length;
	size_t Capacity;
	char Buffer[];
} TextLine;

typedef struct TEXTBUFFER
{
	size_t Count;
	size_t Capacity;
	TextLine **Lines;
} TextBuffer;

void textbuffer_init(TextBuffer *tb);

TextBuffer *textbuffer_create(void);

void textbuffer_print(TextBuffer *tb);

void textbuffer_destroy(TextBuffer *tb);

void textbuffer_replace(TextBuffer *tb, const Selection *sel,
	const char *text, size_t len);

char *textbuffer_get(TextBuffer *tb, const Selection *sel, size_t *len);

static inline const char *textbuffer_get_line(TextBuffer *tb,
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

static inline size_t textbuffer_line_length(TextBuffer *tb, size_t y)
{
	assert(tb);
	assert(y < tb->Count);
	return tb->Lines[y]->Length;
}

static inline size_t textbuffer_lines_count(TextBuffer *tb)
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
		v *= 2;
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

static void textbuffer_require(TextBuffer *tb, size_t capacity)
{
	if(tb->Capacity < capacity)
	{
		size_t new_size = double_until(tb->Capacity, capacity);
		tb->Lines = _realloc(tb->Lines, new_size * sizeof(*tb->Lines));
		tb->Capacity = capacity;
	}
}

void textbuffer_init(TextBuffer *tb)
{
	tb->Capacity = 3;
	tb->Count = 3;
	tb->Lines = _malloc(tb->Capacity * sizeof(*tb->Lines));
	tb->Lines[0] = textline_create("Hello world", 11);
	tb->Lines[1] = textline_create("This is a test", 14);
	tb->Lines[2] = textline_create("Third Line", 10);
}

TextBuffer *textbuffer_create(void)
{
	TextBuffer *tb = _malloc(sizeof(*tb));
	textbuffer_init(tb);
	return tb;
}

void textbuffer_print(TextBuffer *tb)
{
	size_t i;
	for(i = 0; i < tb->Count; ++i)
	{
		printf("%5d | %.*s\n", (int)(i + 1), (int)(tb->Lines[i]->Length),
			tb->Lines[i]->Buffer);
	}

	printf("\nPrinted %d lines total.\n", (int)(tb->Count));
}

void textbuffer_destroy(TextBuffer *tb)
{
	TextLine **start = tb->Lines;
	TextLine **end = start + tb->Count;
	while(start < end)
	{
		_free(*start);
		++start;
	}

	_free(tb->Lines);
	_free(tb);
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

void textbuffer_replace(TextBuffer *tb, const Selection *sel,
	const char *text, size_t len)
{
	Selection nsel = *sel;
	size_t old_lines;
	size_t new_lines;
	size_t new_capacity;
	size_t x1, y1, x2, y2;

	x1 = nsel.Limits[0].X;
	y1 = nsel.Limits[0].Y;
	x2 = nsel.Limits[1].X;
	y2 = nsel.Limits[1].Y;

	selection_normalize(tb, &nsel);

	old_lines = y2 - y1;
	new_lines = count_char(text, len, '\n');
	new_capacity = tb->Capacity + new_lines - old_lines;
	if(new_lines != old_lines)
	{
		textbuffer_require(tb, new_capacity);
	}



	printf("REPLACE: NL's = %d\n", new_lines);
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

char *textbuffer_get(TextBuffer *tb, const Selection *sel, size_t *len)
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

typedef struct LINE
{
	char *Buffer;
	u32 Capacity;
	u32 Length;
	u32 Cursor;
} Line;

void line_init(Line *line, char *buffer, u32 capacity)
{
	line->Buffer = buffer;
	line->Capacity = capacity;
	line_clear(line);
}

void line_clear(Line *line)
{
	line->Length = 0;
	line->Cursor = 0;
}

void line_char(Line *line, u32 c)
{
	if(line->Length < line->Capacity)
	{
		//memmove(line->Buffer + line->Cursor, );
		line->Buffer[line->Cursor] = c;
		++line->Cursor;
		++line->Length;
	}
}

void line_left(Line *line)
{
	if(line->Cursor > 0)
	{
		--line->Cursor;
	}
}

void line_right(Line *line)
{
	if(line->Cursor < line->Length)
	{
		++line->Cursor;
	}
}

void line_backspace(Line *line)
{
	if(line->Cursor > 0)
	{
		char *p = line->Buffer + line->Cursor;
		size_t len = line->Length - line->Cursor;
		memmove(p - 1, p, len);
		--line->Cursor;
		--line->Length;
	}
}

void line_delete(Line *line)
{
	if(line->Cursor < line->Length)
	{

	}
}

void line_home(Line *line)
{
	line->Cursor = 0;
}

void line_end(Line *line)
{
	line->Cursor = line->Length;
}

void line_key(Line *line, u32 key)
{

}

int main(void)
{
	char *copy = NULL;
	size_t len = 0;

	Selection test = { { { 9, 2 }, { 6, 0 } } };

	TextBuffer *tb = textbuffer_create();

	copy = textbuffer_get(tb, &test, &len);

	printf("Length of selection: %d\n", len);
	printf("Copy of selection: %s\n", copy);

	_free(copy);

	textbuffer_replace(tb, &test, "a\nq\nqq\naaa", 10);

	textbuffer_destroy(tb);
	return 0;
}
