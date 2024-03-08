static u8 first_compare;
static char *search_file, same[sizeof(_fld_buf)];

static u32 dir_iter(char *path, void (*iter)(char *, u32))
{
	DIR *dir;
	struct dirent *dp;

	if(!(dir = opendir(path)))
	{
		return 1;
	}

	while((dp = readdir(dir)))
	{
		iter(dp->d_name, dp->d_type == DT_DIR);
	}

	closedir(dir);
	return 0;
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
	char buf[sizeof(_fld_buf)];
	strcpy(buf, _fld.buf);
	path_dir(buf);
	first_compare = 1;
	search_file = path_file(_fld.buf);
	if(dir_iter(buf, tab_cmpl_callback))
	{
		return;
	}

	if(!first_compare)
	{
		strcpy(search_file, same);
		_fld.cursor = _fld.len = strlen(_fld.buf);
	}
}

static size_t revstrlen(char *p)
{
	size_t cnt = 0;
	--p;
	do
	{
		--p;
		++cnt;
	}
	while(*p);
	return cnt;
}

static void path_dir(char *s)
{
	u32 c;
	char *slash = s;
	for(; (c = *s); ++s)
	{
		if(c == '/')
		{
			slash = s;
		}
	}

	strcpy(slash, "/");
}
