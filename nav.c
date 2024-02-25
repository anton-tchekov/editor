static void ed_dir_load(void)
{
	char buf[MAX_SEARCH_LEN];

	dir_pos = 0;
	dir_offset = 0;
	dir_entries = 0;

	nav_buf[nav_len] = '\0';
	strcpy(buf, nav_base);
	path_dir(buf);

	_free(dir_list);
	dir_list = dir_sorted(buf, &dir_entries);
}

static void ed_nav_open(void)
{
	mode = ED_MODE_NAV;
	ed_dir_load();
	ed_render();
}

static void ed_nav_close(void)
{
	mode = ED_MODE_DEFAULT;
	ed_render();
}

static void ed_goto(void)
{
	nav_buf[0] = ':';
	nav_cursor = 1;
	nav_len = 1;
	ed_nav_open();
}

static void ed_open(void)
{
	nav_len = 0;
	nav_cursor = 0;
	ed_nav_open();
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

static void ed_key_press_nav(u32 key, u32 cp)
{
	switch(key)
	{
	case KEY_UP:
		nav_up();
		break;

	case KEY_DOWN:
		nav_down();
		break;

	case KEY_PAGE_UP:
		nav_page_up();
		break;

	case KEY_PAGE_DOWN:
		nav_page_down();
		break;

	case MOD_CTRL | KEY_HOME:
		nav_first();
		break;

	case MOD_CTRL | KEY_END:
		nav_last();
		break;

	case KEY_LEFT:
		if(nav_cursor > 0)
		{
			--nav_cursor;
		}
		ed_render();
		break;

	case KEY_RIGHT:
		if(nav_cursor < nav_len)
		{
			++nav_cursor;
		}
		ed_render();
		break;

	case KEY_HOME:
		nav_cursor = 0;
		ed_render();
		break;

	case KEY_END:
		nav_cursor = nav_len;
		ed_render();
		break;

	case KEY_RETURN:
	{
		char *p;
		if(!nav_len)
		{
			ed_nav_close();
			break;
		}

		nav_buf[nav_len] = '\0';
		p = memchr(nav_buf, ':', nav_len);
		if(p)
		{
			*p = '\0';
			if(p != nav_buf)
			{
				ed_load(nav_buf);
			}

			++p;
			if(ed_set_lnr(p))
			{
				tb_goto_def(tb, p);
			}

			mode = ED_MODE_DEFAULT;
		}
		else
		{
			ed_load(nav_buf);
		}

		ed_render();
		break;
	}

	case KEY_BACKSPACE:
		if(nav_cursor > 0)
		{
			char *p = nav_buf + nav_cursor;
			memmove(p - 1, p, nav_len - nav_cursor);
			--nav_cursor;
			--nav_len;
			ed_render();
		}
		break;

	case KEY_DELETE:
		if(nav_cursor < nav_len)
		{
			char *p = nav_buf + nav_cursor;
			memmove(p, p + 1,  - nav_cursor - 1);
			--nav_len;
			ed_render();
		}
		break;

	case KEY_TAB:
	{
		char buf[MAX_SEARCH_LEN];
		nav_buf[nav_len] = '\0';
		strcpy(buf, nav_base);
		path_dir(buf);
		first_compare = 1;
		search_file = path_file(nav_buf);
		if(dir_iter(buf, ed_tab_cmpl_callback))
		{
			break;
		}

		if(!first_compare)
		{
			strcpy(search_file, same);
			nav_cursor = nav_len = strlen(nav_buf);
			ed_render();
		}
		break;
	}

	case MOD_CTRL | KEY_G:
	case KEY_ESCAPE:
		ed_nav_close();
		break;

	default:
		if(nav_len >= MAX_SEARCH_LEN - 1)
		{
			break;
		}

		if(isprint(cp))
		{
			char *p = nav_buf + nav_cursor;
			memmove(p + 1, p, nav_len - nav_cursor);
			nav_buf[nav_cursor++] = cp;
			++nav_len;
			ed_render();
		}
		break;
	}
}
