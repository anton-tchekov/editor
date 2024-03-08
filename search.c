static u8
	_search_dir,
	_replace,
	_match_case,
	_whole_words,
	_use_escseq;

static u32 escape_seq(char *out, char *s)
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

	*out = '\0';
	return 0;
}

static void mode_search(void)
{
	_mode = MODE_SEARCH;
	_replace = 0;
	_search_dir = 0;
}

static void mode_search_in_dir(void)
{
	_mode = MODE_SEARCH;
	_replace = 0;
	_search_dir = 1;
}

static void mode_replace(void)
{
	_mode = MODE_SEARCH;
	_replace = 1;
	_search_dir = 0;
}

static void mode_replace_in_dir(void)
{
	_mode = MODE_SEARCH;
	_replace = 1;
	_search_dir = 1;
}

static void search_render(void)
{

}

static void sr_key_press(u32 key, u32 chr)
{
	switch(key)
	{
	case KEY_ESCAPE:
		mode_default();
		break;
	}
}
