static void ed_mode_opened(void)
{
	mode = ED_MODE_OPENED;
	dir_entries = bf_count();
	dir_pos = cur_buf;
	dir_offset = 0;
	nav_down_fix_offset();
}

static void opened_up(void)
{
	nav_up();
	bf_switch_id(dir_pos);
}

static void opened_down(void)
{
	nav_down();
	bf_switch_id(dir_pos);
}

static void opened_page_up(void)
{
	nav_page_up();
	bf_switch_id(dir_pos);
}

static void opened_page_down(void)
{
	nav_page_down();
	bf_switch_id(dir_pos);
}

static void opened_first(void)
{
	nav_first();
	bf_switch_id(dir_pos);
}

static void opened_last(void)
{
	nav_last();
	bf_switch_id(dir_pos);
}

static void opened_discard(void)
{
	if(!dir_entries)
	{
		return;
	}

	bf_discard(dir_pos);
	--dir_entries;
	if(!dir_entries)
	{
		tb = NULL;
		return;
	}

	if(dir_offset > 0 && dir_offset + ED_DIR_PAGE >= dir_entries)
	{
		--dir_offset;
	}

	if(dir_pos == dir_entries)
	{
		--dir_pos;
	}

	bf_switch_id(dir_pos);
}

static void ed_key_press_opened(u32 key)
{
	switch(key)
	{
	case MOD_CTRL | KEY_B:
	case KEY_RETURN:
	case KEY_ESCAPE:        ed_mode_default();   break;
	case MOD_CTRL | KEY_S:  ed_save();           break;
	case KEY_UP:            opened_up();         break;
	case KEY_DOWN:          opened_down();       break;
	case KEY_PAGE_UP:       opened_page_up();    break;
	case KEY_PAGE_DOWN:     opened_page_down();  break;
	case KEY_HOME:          opened_first();      break;
	case KEY_END:           opened_last();       break;
	case MOD_CTRL | KEY_W:  opened_discard();    break;
	}
}
