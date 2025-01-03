static void op_filter(void)
{
	vec *term = tf_buf(&_fld);
	vec_clear(&_filt_dir);
	u32 cnt = 0;
	u32 len = vec_num_vecs(&_dir_list);
	for(u32 i = 0; i < len; ++i)
	{
		vec *str = vec_get_vec(&_dir_list, i);
		if(starts_with(str, term))
		{
			vec_push(&_filt_dir, sizeof(vec *), &str);
			++cnt;
		}
	}

	dd_reset(&_dd, cnt);
}

static void op_tab(void)
{
	u32 sl = 0;
	vec *fs = NULL;
	vec *s = tf_buf(&_fld);
	u32 first = 1;
	u32 count = vec_num_vecs(&_dir_list);
	for(u32 i = 0; i < count; ++i)
	{
		vec *cur = vec_get_vec(&_dir_list, i);
		if(starts_with(cur, s))
		{
			if(first)
			{
				first = 0;
				fs = cur;
				sl = fs->len;
			}
			else
			{
				u32 len = 0;
				char *p = vec_str(fs);
				char *q = vec_str(cur);
				for(; len < sl && *p == *q; ++len, ++p, ++q) {}
				sl = len;
			}
		}
	}

	if(!first)
	{
		tf_set(&_fld, vec_str(fs), sl);
	}
}

static void op_dir_reload(void)
{
	// TODO: Check error!!
	vec_destroy(&_dir_list);
	dir_sorted(&_path_buf, &_dir_list);
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
	vec *cur = ((vec **)_filt_dir.data)[_dd.pos];
	if(!strcmp(vec_cstr(cur), "../"))
	{
		path_parent_dir(&_path_buf);
		tf_clear(&_fld);
		op_dir_reload();
	}
	else if(path_is_dir(vec_cstr(cur)))
	{
		vec_strcat(&_path_buf, cur);
		tf_clear(&_fld);
		op_dir_reload();
	}
	else
	{
		vec_strcpy(&_fname_buf, &_path_buf);
		vec_strcat(&_fname_buf, cur);
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
	vec **list = (vec **)_filt_dir.data;
	u32 end = umin(_dd.offset + DD_PAGE, _dd.count);
	for(u32 i = _dd.offset; i < end; ++i, ++y)
	{
		char *s = vec_cstr(list[i]);
		ed_render_line_str(s, 0, y, dd_color(&_dd, i), COLOR_GRAY);
	}

	return y;
}

static u32 op_render(void)
{
	nav_title_render("Open");
	tf_render(&_fld, 1, 1, "Filter: ");
	return op_dir_render(2);
}
