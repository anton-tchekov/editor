static void mode_opened(void)
{
	_mode = ED_MODE_OPENED;
	_dd.count = bf_count();
	_dd.pos = _cur_buf;
	_dd.offset = 0;
	dropdown_down_fix_offset(&_dd);
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
		_tb = NULL;
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
	switch(key)
	{
	case KEY_ESCAPE:
	case KEY_RETURN:       mode_default();                  break;
	case MOD_CTRL | KEY_S: ed_save();                       break;
	case MOD_CTRL | KEY_W: opened_discard(&_dd);   break;
	case MOD_CTRL | KEY_O: mode_open();                     break;
	case KEY_UP:           opened_up(&_dd);        break;
	case KEY_DOWN:         opened_down(&_dd);      break;
	case KEY_PAGE_UP:      opened_page_up(&_dd);   break;
	case KEY_PAGE_DOWN:    opened_page_down(&_dd); break;
	case KEY_HOME:         opened_first(&_dd);     break;
	case KEY_END:          opened_last(&_dd);      break;
	}
}

static void opened_render_title(void)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d buffer%s - %d unsaved",
		_dd.count,
		(_dd.count == 1 ? "" : "s"),
		bf_num_unsaved());

	ed_render_line_str(buf, 0, 0, ptp(PT_FG, PT_INFO));
}

static u32 opened_render(void)
{
	u32 i, y, end;
	opened_render_title();
	end = umin(_dd.offset + DROPDOWN_PAGE, _dd.count);
	for(y = 1, i = _dd.offset; i < end; ++i, ++y)
	{
		textbuf *t = bf_get(i);
		u32 color = dropdown_color(&_dd, i);
		screen_set(0, y, screen_pack(t->modified ? '*' : ' ', color));
		ed_render_line_str(t->filename, 1, y, color);
	}

	return y;
}
