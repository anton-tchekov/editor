static void ob_open(void)
{
	_mode = MODE_OPENED;
	dd_preset(&_dd, _cur_buf, bf_count());
}

static void ob_up(dd *d)
{
	dd_up(d);
	bf_switch_id(d->pos);
}

static void ob_down(dd *d)
{
	dd_down(d);
	bf_switch_id(d->pos);
}

static void ob_page_up(dd *d)
{
	dd_page_up(d);
	bf_switch_id(d->pos);
}

static void ob_page_down(dd *d)
{
	dd_page_down(d);
	bf_switch_id(d->pos);
}

static void ob_first(dd *d)
{
	dd_first(d);
	bf_switch_id(d->pos);
}

static void ob_last(dd *d)
{
	dd_last(d);
	bf_switch_id(d->pos);
}

static void ob_discard(dd *d)
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

static void ob_key(u32 key)
{
	switch(key)
	{
	case KEY_ESCAPE:
	case KEY_RETURN:       mode_default();     break;
	case MOD_CTRL | KEY_S: ed_save();          break;
	case MOD_CTRL | KEY_W: ob_discard(&_dd);   break;
	case MOD_CTRL | KEY_T:
	case MOD_CTRL | KEY_N: ed_new();           break;
	case MOD_CTRL | KEY_O: mode_open();        break;
	case KEY_UP:           ob_up(&_dd);        break;
	case KEY_DOWN:         ob_down(&_dd);      break;
	case KEY_PAGE_UP:      ob_page_up(&_dd);   break;
	case KEY_PAGE_DOWN:    ob_page_down(&_dd); break;
	case KEY_HOME:         ob_first(&_dd);     break;
	case KEY_END:          ob_last(&_dd);      break;
	}
}

static u32 ob_render(void)
{
	char buf[64];
	snprintf(buf, sizeof(buf), "%d buffer%s - %d unsaved",
		_dd.count, (_dd.count == 1 ? "" : "s"), bf_num_unsaved());

	ed_render_line_str(buf, 0, 0, COLOR_FG, COLOR_INFO);
	u32 end = umin(_dd.offset + DD_PAGE, _dd.count);
	u32 y = 1;
	for(u32 i = _dd.offset; i < end; ++i, ++y)
	{
		textbuf *t = bf_get(i);
		u32 fg = dd_color(&_dd, i);
		render_char(0, y, t->modified ? '*' : ' ', fg, COLOR_GRAY);
		ed_render_line_str(vec_cstr(&t->filename), 1, y, fg, COLOR_GRAY);
	}

	return y;
}
