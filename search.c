static u32 escape_seq(char *out, const char *s)
{
	u32 c;
	for(; (c = *s); ++s)
	{
		if(c == '\\')
		{
			++s;
			c = *s;
			switch(c)
			{
			case 't':  c = '\t'; break;
			case 'n':  c = '\n'; break;
			case '\\': c = '\\'; break;
			case '\"': c = '\"'; break;
			case '\'': c = '\''; break;
			default: return 1;
			}
		}

		*out++ = c;
	}

	return 0;
}

static void mode_search(void)
{
	mode = ED_MODE_SEARCH;
	replace = 0;
	search_dir = 0;
}

static void mode_search_in_dir(void)
{
	mode = ED_MODE_SEARCH;
	replace = 0;
	search_dir = 1;
}

static void mode_replace(void)
{
	mode = ED_MODE_SEARCH;
	replace = 1;
	search_dir = 0;
}

static void mode_replace_in_dir(void)
{
	mode = ED_MODE_SEARCH;
	replace = 1;
	search_dir = 1;
}
