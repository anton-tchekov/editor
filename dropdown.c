typedef struct
{
	u32 pos, count, offset;
} dropdown;

static void dropdown_reset(dropdown *d)
{
	d->pos = 0;
	d->offset = 0;
	d->count = 0;
}

static void dropdown_up_fix_offset(dropdown *d)
{
	if(d->pos < d->offset)
	{
		d->offset = d->pos;
	}
}

static void dropdown_down_fix_offset(dropdown *d)
{
	if(d->pos >= d->offset + DROPDOWN_PAGE)
	{
		d->offset = (d->pos < DROPDOWN_PAGE) ? 0 : (d->pos - DROPDOWN_PAGE + 1);
	}
}

static void dropdown_up(dropdown *d)
{
	if(d->pos > 0)
	{
		--d->pos;
		dropdown_up_fix_offset(d);
	}
}

static void dropdown_down(dropdown *d)
{
	if(d->pos < d->count - 1)
	{
		++d->pos;
		dropdown_down_fix_offset(d);
	}
}

static void dropdown_page_up(dropdown *d)
{
	if(d->pos == 0)
	{
		return;
	}

	if(d->pos > DROPDOWN_PAGE)
	{
		d->pos -= DROPDOWN_PAGE;
		dropdown_up_fix_offset(d);
	}
	else
	{
		d->pos = 0;
		d->offset = 0;
	}
}

static void dropdown_page_down(dropdown *d)
{
	if(d->pos < d->count - 1)
	{
		d->pos += DROPDOWN_PAGE;
		if(d->pos > d->count - 1)
		{
			d->pos = d->count - 1;
		}

		dropdown_down_fix_offset(d);
	}
}

static void dropdown_first(dropdown *d)
{
	if(d->pos > 0)
	{
		d->pos = 0;
		d->offset = 0;
	}
}

static void dropdown_last(dropdown *d)
{
	if(d->pos < d->count - 1)
	{
		d->pos = d->count - 1;
		dropdown_down_fix_offset(d);
	}
}

static void dropdown_key(dropdown *d, u32 key)
{
	switch(key)
	{
	case KEY_UP:              dropdown_up(d);        break;
	case KEY_DOWN:            dropdown_down(d);      break;
	case KEY_PAGE_UP:         dropdown_page_up(d);   break;
	case KEY_PAGE_DOWN:       dropdown_page_down(d); break;
	case MOD_CTRL | KEY_HOME: dropdown_first(d);     break;
	case MOD_CTRL | KEY_END:  dropdown_last(d);      break;
	}
}
