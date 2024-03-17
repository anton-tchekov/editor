static u8 _sv_focus;

static void save_as_dir_reload(void)
{
	_free(_dir_list);
	_dir_list = dir_sorted(_path_buf, &_dir_count);
	dd_reset(&_dd, _dir_count);
	_sv_focus = 1;
}

static void mode_save_as(void)
{
	_mode = MODE_SAVE_AS;
	tf_clear(&_fld);
	save_as_dir_reload();
}

static void ed_save_as(void)
{
	u32 len;
	char *buf;

	buf = tb_export(_tb, &len);
	if(file_write(_fname_buf, buf, len))
	{
		msg_show(MSG_ERROR, "Writing file failed");
	}
	else
	{
		msg_show(MSG_INFO, "File saved");
		tb_change_filename(_tb, _fname_buf);
		_tb->modified = 0;
		_tb->exists = 1;
		bf_close_other(_fname_buf, _cur_buf);
	}

	_free(buf);
	mode_default();
}

static void save_as_confirm(u32 yes)
{
	if(yes)
	{
		ed_save_as();
	}
	else
	{
		mode_default();
	}
}

static void save_as_path(void)
{
	if(bf_opened_and_modified(_fname_buf))
	{
		msg_show(MSG_ERROR, "Target has unsaved changes in editor!");
		return;
	}

	if(file_exists(_fname_buf))
	{
		cf_open(save_as_confirm, "Overwrite existing %s? [Y/N]", _fname_buf);
		return;
	}

	ed_save_as();
}

static void save_as_dir_return(void)
{
	char *cur;

	cur = _dir_list[_dd.pos];
	if(!strcmp(cur, "../"))
	{
		path_parent_dir(_path_buf);
		save_as_dir_reload();
	}
	else if(path_is_dir(cur))
	{
		strcat(_path_buf, cur);
		save_as_dir_reload();
	}
	else
	{
		strcpy(_fname_buf, _path_buf);
		strcat(_fname_buf, cur);
		save_as_path();
	}
}

static void save_as_fld_return(void)
{
	if(!tf_len(&_fld))
	{
		return;
	}

	strcpy(_fname_buf, _path_buf);
	strcat(_fname_buf, tf_str(&_fld));
	save_as_path();
}

static void save_as_key(u32 key, u32 c)
{
	if(key == KEY_ESCAPE)
	{
		mode_default();
		return;
	}

	if(_sv_focus)
	{
		switch(key)
		{
		case KEY_DOWN:
			_sv_focus = 0;
			dd_first(&_dd);
			break;

		case MOD_SHIFT | KEY_END:
			_sv_focus = 0;
			dd_last(&_dd);
			break;

		case KEY_PAGE_DOWN:
			_sv_focus = 0;
			dd_page_down(&_dd);
			break;

		case KEY_RETURN:
			save_as_fld_return();
			break;

		default:
			tf_key(&_fld, key, c);
			break;
		}
	}
	else
	{
		switch(key)
		{
		case KEY_RETURN:
			save_as_dir_return();
			break;

		default:
			if(_dd.pos == 0 &&
				(key == KEY_UP || key == KEY_HOME || key == KEY_PAGE_UP))
			{
				_sv_focus = 1;
			}
			else
			{
				dd_key(&_dd, key);
			}
			break;
		}
	}
}

static u32 save_as_dir_render(u32 y)
{
	u32 i, end;

	i = _dd.offset;
	end = umin(i + DD_PAGE, _dd.count);
	if(_sv_focus)
	{
		for(; i < end; ++i, ++y)
		{
			ed_render_line_str(_dir_list[i], 0, y, ptp(PT_FG, PT_GRAY));
		}
	}
	else
	{
		for(; i < end; ++i, ++y)
		{
			ed_render_line_str(_dir_list[i], 0, y, dd_color(&_dd, i));
		}
	}

	return y;
}

static u32 save_as_render(void)
{
	nav_title_render("Save As");
	tf_render(&_fld, 1, _sv_focus, "Filename: ");
	return save_as_dir_render(2);
}
