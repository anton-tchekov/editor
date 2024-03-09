typedef struct
{
	u32 x, y;
} cursor;

typedef struct
{
	cursor c[2];
} selection;

static void cursor_zero(cursor *c)
{
	memset(c, 0, sizeof(*c));
}

static void cursor_swap(cursor *a, cursor *b)
{
	cursor temp;

	temp = *a;
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

static void sel_norm(selection *sel)
{
	if(cursor_unordered(&sel->c[0], &sel->c[1]))
	{
		cursor_swap(&sel->c[0], &sel->c[1]);
	}
}

static cursor *sel_last(selection *sel)
{
	return cursor_unordered(&sel->c[0], &sel->c[1]) ? &sel->c[1] : &sel->c[0];
}
