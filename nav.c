static void ed_dir_load(void)
{
	char buf[MAX_SEARCH_LEN];

	nav_cursor = nav_len;

	dir_pos = 0;
	dir_offset = 0;
	dir_entries = 0;

	nav_buf[nav_len] = '\0';
	strcpy(buf, nav_buf);
	path_dir(buf);
	_free(dir_list);
	dir_list = dir_sorted(buf, &dir_entries);
}

static void ed_open(void)
{
	mode = ED_MODE_OPEN;
	ed_dir_load();
}

static void ed_goto(void)
{
	mode = ED_MODE_GOTO;
	nav_cursor = nav_len = 0;
}

static void ed_save_as(void)
{
	mode = ED_MODE_SAVE_AS;
	ed_dir_load();
}

static void ed_tab_cmpl_callback(const char *fname, u32 is_dir)
{
	if(!strcmp(fname, ".") || !strcmp(fname, ".."))
	{
		return;
	}

	if(starts_with(fname, search_file))
	{
		if(first_compare)
		{
			first_compare = 0;
			strcpy(same, fname);
			if(is_dir)
			{
				strcat(same, "/");
			}
		}
		else
		{
			const char *q = fname;
			char *p = same;
			while(*p == *q) { ++p; ++q; }
			*p = '\0';
		}
	}
}

static u32 ed_set_lnr(const char *s)
{
	u32 lnr = conv_lnr_str(s);
	if(!lnr)
	{
		return 1;
	}

	tb_gotoxy(tb, 0, lnr - 1);
	return 0;
}

static void nav_up_fix_offset(void)
{
	if(dir_pos < dir_offset)
	{
		dir_offset = dir_pos;
	}
}

static void nav_down_fix_offset(void)
{
	if(dir_pos >= dir_offset + ED_DIR_PAGE)
	{
		dir_offset = (dir_pos < ED_DIR_PAGE) ? 0 : (dir_pos - ED_DIR_PAGE + 1);
	}
}

static void nav_up(void)
{
	if(dir_pos > 0)
	{
		--dir_pos;
		nav_up_fix_offset();
	}
}

static void nav_down(void)
{
	if(dir_pos < dir_entries - 1)
	{
		++dir_pos;
		nav_down_fix_offset();
	}
}

static void nav_page_up(void)
{
	if(dir_pos == 0)
	{
		return;
	}

	if(dir_pos > ED_DIR_PAGE)
	{
		dir_pos -= ED_DIR_PAGE;
		nav_up_fix_offset();
	}
	else
	{
		dir_pos = 0;
		dir_offset = 0;
	}
}

static void nav_page_down(void)
{
	if(dir_pos < dir_entries - 1)
	{
		dir_pos += ED_DIR_PAGE;
		if(dir_pos > dir_entries - 1)
		{
			dir_pos = dir_entries - 1;
		}
		nav_down_fix_offset();
	}
}

static void nav_first(void)
{
	if(dir_pos > 0)
	{
		dir_pos = 0;
		dir_offset = 0;
	}
}

static void nav_last(void)
{
	if(dir_pos < dir_entries - 1)
	{
		dir_pos = dir_entries - 1;
		nav_down_fix_offset();
	}
}

static void nav_left(void)
{
	if(nav_cursor > 0)
	{
		--nav_cursor;
	}
}

static void nav_right(void)
{
	if(nav_cursor < nav_len)
	{
		++nav_cursor;
	}
}

static void nav_home(void)
{
	nav_cursor = 0;
}

static void nav_end(void)
{
	nav_cursor = nav_len;
}

static void nav_backspace(void)
{
	if(nav_cursor > 0)
	{
		char *p = nav_buf + nav_cursor;
		memmove(p - 1, p, nav_len - nav_cursor);
		--nav_cursor;
		--nav_len;
	}
}

static void nav_delete(void)
{
	if(nav_cursor < nav_len)
	{
		char *p = nav_buf + nav_cursor;
		memmove(p, p + 1,  - nav_cursor - 1);
		--nav_len;
	}
}

static void nav_char(u32 c)
{
	if(nav_len >= MAX_SEARCH_LEN - 1)
	{
		return;
	}

	if(isprint(c))
	{
		char *p = nav_buf + nav_cursor;
		memmove(p + 1, p, nav_len - nav_cursor);
		nav_buf[nav_cursor++] = c;
		++nav_len;
	}
}

static void open_return(void)
{
	if(!nav_len)
	{
		ed_mode_default();
		return;
	}

	nav_buf[nav_len] = '\0';
	ed_load(nav_buf);
}

static void goto_return(void)
{
	nav_buf[nav_len] = '\0';
	if(ed_set_lnr(nav_buf))
	{
		tb_goto_def(tb, nav_buf);
	}

	ed_mode_default();
}

static void save_as_return(void)
{

}

static void load_save_tab(void)
{
	char buf[MAX_SEARCH_LEN];
	nav_buf[nav_len] = '\0';
	strcpy(buf, nav_buf);
	path_dir(buf);
	first_compare = 1;
	search_file = path_file(nav_buf);
	if(dir_iter(buf, ed_tab_cmpl_callback))
	{
		return;
	}

	if(!first_compare)
	{
		strcpy(search_file, same);
		nav_cursor = nav_len = strlen(nav_buf);
	}
}

static void ed_key_press_open(u32 key, u32 c)
{
	switch(key)
	{
	case KEY_UP:                    nav_up();          break;
	case KEY_DOWN:                  nav_down();        break;
	case KEY_PAGE_UP:               nav_page_up();     break;
	case KEY_PAGE_DOWN:             nav_page_down();   break;
	case MOD_CTRL | KEY_HOME:       nav_first();       break;
	case MOD_CTRL | KEY_END:        nav_last();        break;
	case KEY_LEFT:                  nav_left();        break;
	case KEY_RIGHT:                 nav_right();       break;
	case KEY_HOME:                  nav_home();        break;
	case KEY_END:                   nav_end();         break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                open_return();     break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:             nav_backspace();   break;
	case KEY_DELETE:                nav_delete();      break;
	case KEY_TAB:                   load_save_tab();   break;
	case MOD_CTRL | KEY_G:
	case KEY_ESCAPE:                ed_mode_default(); break;
	default:                        nav_char(c);       break;
	}
}

static void ed_key_press_goto(u32 key, u32 c)
{
	switch(key)
	{
	case KEY_UP:                    nav_up();          break;
	case KEY_DOWN:                  nav_down();        break;
	case KEY_PAGE_UP:               nav_page_up();     break;
	case KEY_PAGE_DOWN:             nav_page_down();   break;
	case MOD_CTRL | KEY_HOME:       nav_first();       break;
	case MOD_CTRL | KEY_END:        nav_last();        break;
	case KEY_LEFT:                  nav_left();        break;
	case KEY_RIGHT:                 nav_right();       break;
	case KEY_HOME:                  nav_home();        break;
	case KEY_END:                   nav_end();         break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                goto_return();     break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:             nav_backspace();   break;
	case KEY_DELETE:                nav_delete();      break;
	case KEY_ESCAPE:                ed_mode_default(); break;
	default:                        nav_char(c);       break;
	}
}

static void ed_key_press_save_as(u32 key, u32 c)
{
	switch(key)
	{
	case KEY_UP:                    nav_up();          break;
	case KEY_DOWN:                  nav_down();        break;
	case KEY_PAGE_UP:               nav_page_up();     break;
	case KEY_PAGE_DOWN:             nav_page_down();   break;
	case MOD_CTRL | KEY_HOME:       nav_first();       break;
	case MOD_CTRL | KEY_END:        nav_last();        break;
	case KEY_LEFT:                  nav_left();        break;
	case KEY_RIGHT:                 nav_right();       break;
	case KEY_HOME:                  nav_home();        break;
	case KEY_END:                   nav_end();         break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                save_as_return();  break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:             nav_backspace();   break;
	case KEY_DELETE:                nav_delete();      break;
	case KEY_TAB:                   load_save_tab();   break;
	case KEY_ESCAPE:                ed_mode_default(); break;
	default:                        nav_char(c);       break;
	}
}
