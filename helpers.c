enum
{
	CT_SPACE,
	CT_IDENTIFIER,
	CT_OTHER
};

static void allocfail(void)
{
	fprintf(stderr, "Memory allocation failure\n");
	exit(1);
}

static void *_malloc(size_t size)
{
	void *p = malloc(size);
	if(!p) { allocfail(); }
	return p;
}

static void *_realloc(void *p, size_t size)
{
	p = realloc(p, size);
	if(!p) { allocfail(); }
	return p;
}

static u32 is_ident(u32 c)
{
	return c == '_' || isalnum(c);
}

static u32 is_bin(u32 c)
{
	return c == '0' || c == '1';
}

static u32 is_oct(u32 c)
{
	return c >= '0' && c <= '7';
}

static u32 char_type(u32 c)
{
	if(isspace(c))
	{
		return CT_SPACE;
	}
	else if(is_ident(c))
	{
		return CT_IDENTIFIER;
	}

	return CT_OTHER;
}

static u32 conv_lnr_str(const char *s)
{
	u32 c;
	u32 lnr = 0;
	for(; (c = *s); ++s)
	{
		if(!isdigit(c))
		{
			return 0;
		}

		lnr = 10 * lnr + (c - '0');
	}

	return lnr;
}

static u32 dec_digit_cnt(u32 n)
{
	u32 cnt = 0;
	while(n)
	{
		++cnt;
		n /= 10;
	}

	return cnt;
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

static void linenr_str(char *s, u32 n, u32 width)
{
	u32 i = 0;
	do
	{
		s[i++] = n % 10 + '0';
		n /= 10;
	}
	while(n > 0);

	while(i < width)
	{
		s[i++] = ' ';
	}

	s[i] = '\0';
	reverse(s, width);
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

	if(slash == s)
	{
		slash[0] = '.';
	}

	slash[1] = '\0';
}

static char *path_file(char *s)
{
	u32 c;
	char *slash = s;
	for(; (c = *s); ++s)
	{
		if(c == '/')
		{
			slash = s + 1;
		}
	}

	return slash;
}

static u32 starts_with(const char *str, const char *prefix)
{
	while(*prefix)
	{
		if(*prefix++ != *str++)
		{
			return 0;
		}
	}

	return 1;
}

static u32 match_part(const char *str, const char *cmp, u32 len)
{
	return len == strlen(cmp) && !strncmp(str, cmp, len);
}

static u32 is_text(const char *s, size_t len)
{
	u32 c;
	const char *end;
	for(end = s + len; s < end; ++s)
	{
		c = *s;
		if(!isprint(c) && c != '\n' && c != '\t')
		{
			return 1;
		}
	}

	return 0;
}
