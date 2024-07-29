typedef struct
{
	u32 pos, count, offset;
} dd;

static void dd_down_fix_offset(dd *d)
{
	if(d->pos >= d->offset + DD_PAGE)
	{
		d->offset = (d->pos < DD_PAGE) ? 0 : (d->pos - DD_PAGE + 1);
	}
}

static void dd_preset(dd *d, u32 pos, u32 cnt)
{
	d->count = cnt;
	d->pos = pos;
	d->offset = 0;
	dd_down_fix_offset(d);
}

static void dd_reset(dd *d, u32 cnt)
{
	d->pos = 0;
	d->offset = 0;
	d->count = cnt;
}

static void dd_up_fix_offset(dd *d)
{
	if(d->pos < d->offset)
	{
		d->offset = d->pos;
	}
}

static void dd_up(dd *d)
{
	if(d->pos > 0)
	{
		--d->pos;
		dd_up_fix_offset(d);
	}
}

static void dd_down(dd *d)
{
	if(d->pos < d->count - 1)
	{
		++d->pos;
		dd_down_fix_offset(d);
	}
}

static void dd_page_up(dd *d)
{
	if(d->pos == 0)
	{
		return;
	}

	if(d->pos > DD_PAGE)
	{
		d->pos -= DD_PAGE;
		dd_up_fix_offset(d);
	}
	else
	{
		d->pos = 0;
		d->offset = 0;
	}
}

static void dd_page_down(dd *d)
{
	if(d->pos < d->count - 1)
	{
		d->pos += DD_PAGE;
		if(d->pos > d->count - 1)
		{
			d->pos = d->count - 1;
		}

		dd_down_fix_offset(d);
	}
}

static void dd_first(dd *d)
{
	if(d->pos > 0)
	{
		d->pos = 0;
		d->offset = 0;
	}
}

static void dd_last(dd *d)
{
	if(d->pos < d->count - 1)
	{
		d->pos = d->count - 1;
		dd_down_fix_offset(d);
	}
}

static void dd_key(dd *d, u32 key)
{
	switch(key)
	{
	case KEY_UP:              dd_up(d);        break;
	case KEY_DOWN:            dd_down(d);      break;
	case KEY_PAGE_UP:         dd_page_up(d);   break;
	case KEY_PAGE_DOWN:       dd_page_down(d); break;
	case MOD_CTRL | KEY_HOME: dd_first(d);     break;
	case MOD_CTRL | KEY_END:  dd_last(d);      break;
	}
}

static u32 dd_color(dd *d, u32 i)
{
	return (i == d->pos) ? COLOR_COMMENT : COLOR_FG;
}
