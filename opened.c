static void mode_opened(void)
{
	_mode = MODE_OPENED;
	dd_preset(&_dd, _cur_buf, bf_count());
}

static void opened_up(dd *d)
{
	dd_up(d);
	bf_switch_id(d->pos);
}

static void opened_down(dd *d)
{
	dd_down(d);
	bf_switch_id(d->pos);
}

static void opened_page_up(dd *d)
{
	dd_page_up(d);
	bf_switch_id(d->pos);
}

static void opened_page_down(dd *d)
{
	dd_page_down(d);
	bf_switch_id(d->pos);
}

static void opened_first(dd *d)
{
	dd_first(d);
	bf_switch_id(d->pos);
}

static void opened_last(dd *d)
{
	dd_last(d);
	bf_switch_id(d->pos);
}

static void opened_discard(dd *d)
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

	if(d->offset > 0 && d->offset + DD_PAGE >= d->count)
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
	case KEY_RETURN:       mode_default();         break;
	case MOD_CTRL | KEY_S: ed_save();              break;
	case MOD_CTRL | KEY_W: opened_discard(&_dd);   break;
	case MOD_CTRL | KEY_O: mode_open();            break;
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
		_dd.count, (_dd.count == 1 ? "" : "s"), bf_num_unsaved());

	ed_render_line_str(buf, 0, 0, ptp(PT_FG, PT_INFO));
}

static u32 opened_render(void)
{
	u32 i, y, end;
	opened_render_title();
	end = umin(_dd.offset + DD_PAGE, _dd.count);
	for(y = 1, i = _dd.offset; i < end; ++i, ++y)
	{
		textbuf *t = bf_get(i);
		u32 color = dd_color(&_dd, i);
		screen_set(0, y, screen_pack(t->modified ? '*' : ' ', color));
		ed_render_line_str(t->filename, 1, y, color);
	}

	return y;
}
