/* Previously used, currently unused code */
static size_t revstrlen(char *p)
{
	size_t cnt;

	cnt = 0;
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
	char *slash;

	slash = s;
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

static void reverse(char *s, u32 len)
{
	u32 i, j, c;

	for(i = 0, j = len - 1; i < j; ++i, --j)
	{
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

static void rect(u32 x, u32 y, u32 w, u32 h, u32 color)
{
	u32 x0;
	u32 *line;

	assert(x < _gfx_width);
	assert(y < _gfx_height);
	assert(w < _gfx_width);
	assert(h < _gfx_height);
	assert((x + w) <= _gfx_width);
	assert((y + h) <= _gfx_height);

	line = _pixels + y * _gfx_width + x;
	while(h)
	{
		for(x0 = 0; x0 < w; ++x0)
		{
			line[x0] = color;
		}

		line += _gfx_width;
		--h;
	}
}
