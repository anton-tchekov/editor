static void mode_save_as(void)
{
	_mode = ED_MODE_SAVE_AS;
	nav_dir_reload();
}

static void ed_save_as(void)
{
	u32 len;
	char *buf = tb_export(_tb, &len);
	if(file_write(_fld.buf, buf, len))
	{
		msg_show(MSG_ERROR, "Writing file failed");
	}
	else
	{
		msg_show(MSG_INFO, "File saved");
		tb_change_filename(_tb, _fld.buf);
		_tb->modified = 0;
		_tb->exists = 1;
		bf_close_other(_fld.buf, _cur_buf);
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

static void save_as_return(void)
{
	if(bf_opened_and_modified(_fld.buf))
	{
		msg_show(MSG_ERROR, "Selected file has unsaved changes in editor!");
		return;
	}

	if(file_exists(_fld.buf))
	{
		confirm(save_as_confirm,
			"Overwrite existing file %s? [Y/N]", _fld.buf);
		return;
	}

	ed_save_as();
}

static void save_as_key_press(u32 key, u32 c)
{
	field_key(&_fld, key, c);
	dropdown_key(&_dd, key);
	switch(key)
	{
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:             save_as_return(); break;
	case KEY_ESCAPE:             mode_default();   break;
	}
}

static u32 save_as_render(void)
{
	return nav_render(_dir_list, "Save As", "Filename: ");
}
