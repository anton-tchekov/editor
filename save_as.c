static u8 _focus;

static void save_as_dir_reload(void)
{
	_free(_dir_list);
	_dir_list = dir_sorted(_path_buf, &_dir_count);
	dropdown_reset(&_dd, _dir_count);
	_focus = 1;
}

static void mode_save_as(void)
{
	_mode = ED_MODE_SAVE_AS;
	field_reset(&_fld);
	save_as_dir_reload();
}

static void ed_save_as(void)
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

static void save_as_confirm(u32 yes)
{
	if(yes)
	{
		ed_save_as();
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
		confirm(save_as_confirm, "Overwrite existing %s? [Y/N]", _fname_buf);
		return;
	}

	ed_save_as();
}

static void save_as_dir_return(void)
{
	char *cur = _dir_list[_dd.pos];
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
	if(!_fld.len)
	{
		return;
	}

	strcpy(_fname_buf, _path_buf);
	strcat(_fname_buf, _fld.buf);
	save_as_path();
}

static void save_as_key_press(u32 key, u32 c)
{
	if(key == KEY_ESCAPE)
	{
		mode_default();
		return;
	}

	if(_focus)
	{
		switch(key)
		{
		case KEY_DOWN:
			_focus = 0;
			dropdown_first(&_dd);
			break;

		case MOD_SHIFT | KEY_END:
			_focus = 0;
			dropdown_last(&_dd);
			break;

		case KEY_PAGE_DOWN:
			_focus = 0;
			dropdown_page_down(&_dd);
			break;

		case KEY_RETURN:
			save_as_fld_return();
			break;

		default:
			field_key(&_fld, key, c);
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
				_focus = 1;
			}
			else
			{
				dropdown_key(&_dd, key);
			}
			break;
		}
	}
}

static u32 save_as_dir_render(u32 y)
{
	u32 i, end;
	end = umin(_dd.offset + DROPDOWN_PAGE, _dd.count);
	i = _dd.offset;
	if(_focus)
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
			ed_render_line_str(_dir_list[i], 0, y, dropdown_color(&_dd, i));
		}
	}

	return y;
}

static u32 save_as_render(void)
{
	nav_title_render("Save As");
	field_render(&_fld, 1, _focus, "Filename: ");
	return save_as_dir_render(2);
}
