static void mode_opened(void)
{
	mode = ED_MODE_OPENED;
	dropdown_nav.count = bf_count();
	dropdown_nav.pos = cur_buf;
	dropdown_nav.offset = 0;
	dropdown_down_fix_offset(&dropdown_nav);
}

static void opened_up(dropdown *d)
{
	dropdown_up(d);
	bf_switch_id(d->pos);
}

static void opened_down(dropdown *d)
{
	dropdown_down(d);
	bf_switch_id(d->pos);
}

static void opened_page_up(dropdown *d)
{
	dropdown_page_up(d);
	bf_switch_id(d->pos);
}

static void opened_page_down(dropdown *d)
{
	dropdown_page_down(d);
	bf_switch_id(d->pos);
}

static void opened_first(dropdown *d)
{
	dropdown_first(d);
	bf_switch_id(d->pos);
}

static void opened_last(dropdown *d)
{
	dropdown_last(d);
	bf_switch_id(d->pos);
}

static void opened_discard(dropdown *d)
{
	if(!d->count)
	{
		return;
	}

	bf_discard(d->pos);
	--d->count;
	if(!d->count)
	{
		tb = NULL;
		return;
	}

	if(d->offset > 0 && d->offset + DROPDOWN_PAGE >= d->count)
	{
		--d->offset;
	}

	if(d->pos == d->count)
	{
		--d->pos;
	}

	bf_switch_id(d->pos);
}

static void opened_key_press(u32 key)
{
	if(key == KEY_RETURN || key == KEY_ESCAPE)
	{
		mode_default();
		return;
	}

	if(key == (MOD_CTRL | KEY_S))
	{
		ed_save();
		return;
	}

	switch(key)
	{
	case KEY_UP:           opened_up(&dropdown_nav);        break;
	case KEY_DOWN:         opened_down(&dropdown_nav);      break;
	case KEY_PAGE_UP:      opened_page_up(&dropdown_nav);   break;
	case KEY_PAGE_DOWN:    opened_page_down(&dropdown_nav); break;
	case KEY_HOME:         opened_first(&dropdown_nav);     break;
	case KEY_END:          opened_last(&dropdown_nav);      break;
	case MOD_CTRL | KEY_W: opened_discard(&dropdown_nav);   break;
	}
}
