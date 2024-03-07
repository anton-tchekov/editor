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

static void opened_render_title(void)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d buffer%s - %d unsaved",
		dropdown_nav.count, (dropdown_nav.count == 1 ? "" : "s"),
		bf_num_unsaved());

	ed_render_line_str(buf, 0, 0,
		screen_color(COLOR_TABLE_FG, COLOR_TABLE_INFO));
}

static u32 opened_render(void)
{
	u32 i, y, end;
	opened_render_title();
	end = umin(dropdown_nav.offset + DROPDOWN_PAGE, dropdown_nav.count);
	for(y = 1, i = dropdown_nav.offset; i < end; ++i, ++y)
	{
		textbuf *t = bf_get(i);
		u32 color = dropdown_color(&dropdown_nav, i);
		screen_set(0, y, screen_pack(t->modified ? '*' : ' ', color));
		ed_render_line_str(t->filename, 1, y, color);
	}

	return y;
}
