static void open_filter(void)
{
	u32 i, cnt;
	char *s;

	s = tf_str(&_fld);
	vector_clear(&_filt_dir);
	for(cnt = 0, i = 0; i < _dir_count; ++i)
	{
		if(starts_with(_dir_list[i], s))
		{
			vector_push(&_filt_dir, sizeof(char *), _dir_list + i);
			++cnt;
		}
	}

	dd_reset(&_dd, cnt);
}

static void open_tab(void)
{
	u32 i, first, sl;
	char *s, *fs;

	s = tf_str(&_fld);
	first = 1;
	for(i = 0; i < _dir_count; ++i)
	{
		char *cur;

		cur = _dir_list[i];
		if(starts_with(cur, s))
		{
			if(first)
			{
				first = 0;
				fs = cur;
				sl = strlen(fs);
			}
			else
			{
				char *p;
				u32 len;

				p = fs;
				for(len = 0; len < sl && *p == *cur; ++len, ++p, ++cur) {}
				sl = len;
			}
		}
	}

	if(!first)
	{
		tf_set(&_fld, fs, sl);
	}
}

static void open_dir_reload(void)
{
	_free(_dir_list);
	_dir_list = dir_sorted(_path_buf, &_dir_count);
	open_filter();
}

static void mode_open(void)
{
	_mode = MODE_OPEN;
	tf_clear(&_fld);
	open_dir_reload();
}

static void open_return(void)
{
	char *cur;

	cur = ((char **)vector_data(&_filt_dir))[_dd.pos];
	if(!strcmp(cur, "../"))
	{
		path_parent_dir(_path_buf);
		tf_clear(&_fld);
		open_dir_reload();
	}
	else if(path_is_dir(cur))
	{
		strcat(_path_buf, cur);
		tf_clear(&_fld);
		open_dir_reload();
	}
	else
	{
		strcpy(_fname_buf, _path_buf);
		strcat(_fname_buf, cur);
		ed_load(_fname_buf);
	}
}

static void open_key(u32 key, u32 c)
{
	dd_key(&_dd, key);
	if(tf_key(&_fld, key, c))
	{
		open_filter();
	}

	switch(key & 0xFF)
	{
	case KEY_TAB:    open_tab();     break;
	case KEY_RETURN: open_return();  break;
	case KEY_ESCAPE: mode_default(); break;
	}
}

static u32 open_dir_render(u32 y)
{
	char **list;
	u32 i, end;

	list = vector_data(&_filt_dir);
	end = umin(_dd.offset + DD_PAGE, _dd.count);
	for(i = _dd.offset; i < end; ++i, ++y)
	{
		ed_render_line_str(list[i], 0, y, dd_color(&_dd, i));
	}

	return y;
}

static u32 open_render(void)
{
	nav_title_render("Open");
	tf_render(&_fld, 1, 1, "Filter: ");
	return open_dir_render(2);
}
