/* Previously used, currently unused code */
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

static u32 starts_with_ic(char *str, char *prefix)
{
	while(*prefix)
	{
		if(tolower(*prefix++) != tolower(*str++))
		{
			return 0;
		}
	}

	return 1;
}
