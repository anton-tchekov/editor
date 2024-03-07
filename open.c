static void open_dir_reload(void)
{
	dropdown_reset(&dropdown_nav);
	_free(dir_list);
	dir_list = dir_sorted(_path_buf, &dropdown_nav.count);
}

static void mode_open(void)
{
	_mode = ED_MODE_OPEN;
	open_dir_reload();
}

static void open_return(void)
{
	if(dropdown_nav.pos == 0)
	{
		path_parent_dir(_path_buf);
		open_dir_reload();
	}
	else
	{
		char *de = dir_list[dropdown_nav.pos];
		if(path_is_dir(de))
		{
			strcat(_path_buf, de);
			open_dir_reload();
		}
		else
		{
			char *end = memchr(_path_buf, '\0', sizeof(_path_buf));
			strcpy(end, de);
			ed_load(_path_buf);
			*end = '\0';
		}
	}
}

static void open_key_press(u32 key, u32 c)
{
	dropdown_key(&dropdown_nav, key);
	field_key(&fld_nav, key, c);
	switch(key & 0xFF)
	{
	case KEY_RETURN: open_return();  break;
	case KEY_ESCAPE: mode_default(); break;
	}
}

static u32 open_render(void)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "Open: %s [%d]",
		_path_buf, dropdown_nav.count - 1);
	ed_render_line_str(buf, 0, 0, ptp(PT_BG, PT_FG));
	ed_render_nav(&fld_nav, 1, "Filter: ");
	return ed_render_dir(2);
}
