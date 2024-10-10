static u8 _sv_focus;

static void sv_dir_reload(void)
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
	sv_dir_reload();
}

static void sv_write(void)
{
	u32 len;
	char *buf = tb_export(_tb, &len);
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

static void sv_confirm(u32 yes)
{
	if(yes)
	{
		sv_write();
	}
	else
	{
		mode_default();
	}
}

static void sv_path(void)
{
	if(bf_opened_and_modified(_fname_buf))
	{
		msg_show(MSG_ERROR, "Target has unsaved changes in editor!");
		return;
	}

	if(file_exists(_fname_buf))
	{
		cf_open(sv_confirm, "Overwrite existing %s? [Y/N]", _fname_buf);
		return;
	}

	sv_write();
}

static void sv_dir_return(void)
{
	char *cur = _dir_list[_dd.pos];
	if(!strcmp(cur, "../"))
	{
		path_parent_dir(_path_buf);
		sv_dir_reload();
	}
	else if(path_is_dir(cur))
	{
		strcat(_path_buf, cur);
		sv_dir_reload();
	}
	else
	{
		strcpy(_fname_buf, _path_buf);
		strcat(_fname_buf, cur);
		sv_path();
	}
}

static void sv_fld_return(void)
{
	if(!tf_len(&_fld))
	{
		return;
	}

	strcpy(_fname_buf, _path_buf);
	strcat(_fname_buf, tf_str(&_fld));
	sv_path();
}

static void sv_key(u32 key, u32 c)
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
			sv_fld_return();
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
			sv_dir_return();
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

static u32 sv_dir_render(u32 y)
{
	u32 i, end;

	i = _dd.offset;
	end = umin(i + DD_PAGE, _dd.count);
	if(_sv_focus)
	{
		for(; i < end; ++i, ++y)
		{
			ed_render_line_str(_dir_list[i], 0, y, COLOR_FG, COLOR_GRAY);
		}
	}
	else
	{
		for(; i < end; ++i, ++y)
		{
			ed_render_line_str(_dir_list[i], 0, y, dd_color(&_dd, i), COLOR_GRAY);
		}
	}

	return y;
}

static u32 sv_render(void)
{
	nav_title_render("Save As");
	tf_render(&_fld, 1, _sv_focus, "Filename: ");
	return sv_dir_render(2);
}
