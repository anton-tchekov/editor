static void mode_save_as(void)
{
	mode = ED_MODE_SAVE_AS;
	ed_dir_load();
}

static void ed_save_as(void)
{
	u32 len;
	char *buf = tb_export(tb, &len);
	if(file_write(fld_nav.buf, buf, len))
	{
		ed_msg(ED_ERROR, "Writing file failed");
	}
	else
	{
		ed_msg(ED_INFO, "File saved");
		tb_change_filename(tb, fld_nav.buf);
		tb->modified = 0;
		tb->exists = 1;
		bf_close_other(fld_nav.buf, cur_buf);
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
	if(bf_opened_and_modified(fld_nav.buf))
	{
		ed_msg(ED_ERROR, "Selected file has unsaved changes in editor!");
		return;
	}

	if(file_exists(fld_nav.buf))
	{
		mode_confirm(save_as_confirm,
			"Overwrite existing file %s? [Y/N]", fld_nav.buf);
		return;
	}

	ed_save_as();
}

static void save_as_key_press(u32 key, u32 c)
{
	field_key(&fld_nav, key, c);
	dropdown_key(&dropdown_nav, key);
	switch(key)
	{
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:             save_as_return(); break;
	case KEY_TAB:                load_save_tab();  break;
	case KEY_ESCAPE:             mode_default();   break;
	}
}
