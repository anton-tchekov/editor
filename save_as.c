static void ed_save_as(void)
{
	mode = ED_MODE_SAVE_AS;
	ed_dir_load();
}

static void save_as_return(void)
{
	mode_confirm("File %s already exists. Overwrite? [Y/N]", "[NAME]");
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
