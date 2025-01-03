static u8 _sv_focus;

static void sv_dir_reload(void)
{
	vec_destroy(&_dir_list);
	dir_sorted(&_path_buf, &_dir_list);
	dd_reset(&_dd, vec_num_vecs(&_dir_list));
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
	char *utf8 = convert_to_utf8(buf, &len);
	_free(buf);
	if(file_write(vec_cstr(&_fname_buf), utf8, len))
	{
		msg_show(MSG_ERROR, "Writing file failed");
	}
	else
	{
		msg_show(MSG_INFO, "File saved");
		tb_change_filename(_tb, &_fname_buf);
		_tb->modified = 0;
		_tb->exists = 1;
		bf_close_other(&_fname_buf, _cur_buf);
	}

	_free(utf8);
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
	if(bf_opened_and_modified(&_fname_buf))
	{
		msg_show(MSG_ERROR, "Target has unsaved changes in editor!");
		return;
	}

	if(file_exists(vec_cstr(&_fname_buf)))
	{
		cf_open(sv_confirm, "Overwrite existing %s? [Y/N]", _fname_buf);
		return;
	}

	sv_write();
}

static void sv_dir_return(void)
{
	vec *cur = vec_get_vec(&_dir_list, _dd.pos);
	if(!strcmp(vec_cstr(cur), "../"))
	{
		path_parent_dir(&_path_buf);
		sv_dir_reload();
	}
	else if(path_is_dir(vec_cstr(cur)))
	{
		vec_strcat(&_path_buf, cur);
		sv_dir_reload();
	}
	else
	{
		vec_strcpy(&_fname_buf, &_path_buf);
		vec_strcat(&_fname_buf, cur);
		sv_path();
	}
}

static void sv_fld_return(void)
{
	if(!tf_len(&_fld))
	{
		return;
	}

	vec_strcpy(&_fname_buf, &_path_buf);
	vec_strcat(&_fname_buf, tf_buf(&_fld));
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
	u32 i = _dd.offset;
	u32 end = umin(i + DD_PAGE, _dd.count);
	for(; i < end; ++i, ++y)
	{
		char *s = vec_cstr(vec_get_vec(&_dir_list, i));
		ed_render_line_str(s, 0, y,
			_sv_focus ? COLOR_FG : dd_color(&_dd, i),
			COLOR_GRAY);
	}

	return y;
}

static u32 sv_render(void)
{
	nav_title_render("Save As");
	tf_render(&_fld, 1, _sv_focus, "Filename: ");
	return sv_dir_render(2);
}
