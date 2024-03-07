static void mode_open(void)
{
	_mode = ED_MODE_OPEN;
	nav_dir_reload();
}

static void open_return(void)
{
	if(_dd.pos == 0)
	{
		path_parent_dir(_path_buf);
		nav_dir_reload();
	}
	else
	{
		char *de = _dir_list[_dd.pos];
		if(path_is_dir(de))
		{
			strcat(_path_buf, de);
			nav_dir_reload();
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
	dropdown_key(&_dd, key);
	field_key(&_fld, key, c);
	switch(key & 0xFF)
	{
	case KEY_RETURN: open_return();  break;
	case KEY_ESCAPE: mode_default(); break;
	}
}

static u32 open_render(void)
{
	return nav_render("Open", "Filter: ");
}
