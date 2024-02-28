static void mode_open(void)
{
	mode = ED_MODE_OPEN;
	ed_dir_load();
}

static void open_return(void)
{
	if(!fld_nav.len)
	{
		mode_default();
		return;
	}

	fld_nav.buf[fld_nav.len] = '\0';
	ed_load(fld_nav.buf);
}

static void open_key_press(u32 key, u32 c)
{
	field_key(&fld_nav, key, c);
	dropdown_key(&dropdown_nav, key);
	switch(key)
	{
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:             open_return();   break;
	case KEY_TAB:                load_save_tab(); break;
	case KEY_ESCAPE:             mode_default();  break;
	}
}
