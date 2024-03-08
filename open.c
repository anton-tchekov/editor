static void open_filter(void)
{
	u32 i, cnt;
	vector_clear(&_filt_dir);
	for(cnt = 0, i = 0; i < _dir_count; ++i)
	{
		if(starts_with_ic(_dir_list[i], _fld.buf))
		{
			vector_push(&_filt_dir, sizeof(char *), _dir_list + i);
			++cnt;
		}
	}

	dropdown_reset(&_dd, cnt);
}

static void open_dir_reload(void)
{
	_free(_dir_list);
	_dir_list = dir_sorted(_path_buf, &_dir_count);
	open_filter();
}

static void mode_open(void)
{
	_mode = ED_MODE_OPEN;
	field_reset(&_fld);
	open_dir_reload();
}

static void open_return(void)
{
	char *cur = ((char **)vector_data(&_filt_dir))[_dd.pos];
	if(!strcmp(cur, "../"))
	{
		path_parent_dir(_path_buf);
		field_reset(&_fld);
		open_dir_reload();
	}
	else if(path_is_dir(cur))
	{
		strcat(_path_buf, cur);
		field_reset(&_fld);
		open_dir_reload();
	}
	else
	{
		char *end = memchr(_path_buf, '\0', sizeof(_path_buf));
		strcpy(end, cur);
		ed_load(_path_buf);
		*end = '\0';
	}
}

static void open_key_press(u32 key, u32 c)
{
	dropdown_key(&_dd, key);
	if(field_key(&_fld, key, c))
	{
		open_filter();
	}

	switch(key & 0xFF)
	{
	case KEY_RETURN: open_return();  break;
	case KEY_ESCAPE: mode_default(); break;
	}
}

static u32 open_dir_render(u32 y)
{
	u32 i, end;
	char **list = vector_data(&_filt_dir);
	end = umin(_dd.offset + DROPDOWN_PAGE, _dd.count);
	for(i = _dd.offset; i < end; ++i, ++y)
	{
		ed_render_line_str(list[i], 0, y, dropdown_color(&_dd, i));
	}

	return y;
}

static u32 open_render(void)
{
	nav_title_render("Open");
	field_render(&_fld, 1, 1, "Filter: ");
	return open_dir_render(2);
}
