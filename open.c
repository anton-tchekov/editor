static void op_filter(void)
{
	char *s = tf_str(&_fld);
	vec_clear(&_filt_dir);
	u32 cnt = 0;
	for(u32 i = 0; i < _dir_count; ++i)
	{
		if(starts_with(_dir_list[i], s))
		{
			vec_push(&_filt_dir, sizeof(char *), _dir_list + i);
			++cnt;
		}
	}

	dd_reset(&_dd, cnt);
}

static void op_tab(void)
{
	u32 sl = 0;
	char *fs = NULL;
	char *s = tf_str(&_fld);
	u32 first = 1;
	for(u32 i = 0; i < _dir_count; ++i)
	{
		char *cur = _dir_list[i];
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
				u32 len;
				char *p = fs;
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

static void op_dir_reload(void)
{
	_free(_dir_list);
	_dir_list = dir_sorted(&_path_buf, &_dir_count);
	op_filter();
}

static void mode_open(void)
{
	_mode = MODE_OPEN;
	tf_clear(&_fld);
	op_dir_reload();
}

static void op_return(void)
{
	char *cur = ((char **)vec_data(&_filt_dir))[_dd.pos];
	if(!strcmp(cur, "../"))
	{
		path_parent_dir(&_path_buf);
		tf_clear(&_fld);
		op_dir_reload();
	}
	else if(path_is_dir(cur))
	{
		vec_cstrcat(&_path_buf, cur);
		tf_clear(&_fld);
		op_dir_reload();
	}
	else
	{
		vec_strcpy(&_fname_buf, &_path_buf);
		vec_cstrcat(&_fname_buf, cur);
		ed_load(&_fname_buf);
	}
}

static void op_key(u32 key, u32 c)
{
	dd_key(&_dd, key);
	if(tf_key(&_fld, key, c))
	{
		op_filter();
	}

	switch(key & 0xFF)
	{
	case KEY_TAB:    op_tab();     break;
	case KEY_RETURN: op_return();  break;
	case KEY_ESCAPE: mode_default(); break;
	}
}

static u32 op_dir_render(u32 y)
{
	char **list = vec_data(&_filt_dir);
	u32 end = umin(_dd.offset + DD_PAGE, _dd.count);
	for(u32 i = _dd.offset; i < end; ++i, ++y)
	{
		ed_render_line_str(list[i], 0, y, dd_color(&_dd, i), COLOR_GRAY);
	}

	return y;
}

static u32 op_render(void)
{
	nav_title_render("Open");
	tf_render(&_fld, 1, 1, "Filter: ");
	return op_dir_render(2);
}
