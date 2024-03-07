/* Common functions for "Load File" and "Save As" file browser */

static u8 first_compare;
static char *search_file, same[sizeof(nav_buf)];

static void ed_dir_load(void)
{
	char buf[sizeof(nav_buf)];
	field_end(&fld_nav);
	dropdown_reset(&dropdown_nav);
	field_add_nt(&fld_nav);
	strcpy(buf, fld_nav.buf);
	path_dir(buf);
	_free(dir_list);
	dir_list = dir_sorted(buf, &dropdown_nav.count);
}

static void tab_cmpl_callback(char *fname, u32 is_dir)
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
			char *q = fname;
			char *p = same;
			while(*p == *q) { ++p; ++q; }
			*p = '\0';
		}
	}
}

static void load_save_tab(void)
{
	char buf[sizeof(nav_buf)];
	field_add_nt(&fld_nav);
	strcpy(buf, fld_nav.buf);
	path_dir(buf);
	first_compare = 1;
	search_file = path_file(fld_nav.buf);
	if(dir_iter(buf, tab_cmpl_callback))
	{
		return;
	}

	if(!first_compare)
	{
		strcpy(search_file, same);
		fld_nav.cursor = fld_nav.len = strlen(fld_nav.buf);
	}
}
