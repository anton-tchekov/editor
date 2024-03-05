enum
{
	CT_SPACE,
	CT_IDENTIFIER,
	CT_OTHER
};

static u32 is_ident(u32 c)
{
	return c == '_' || isalnum(c);
}

static u32 is_asm_ident(u32 c)
{
	return c == '.' || is_ident(c);
}

static u32 is_ident_start(u32 c)
{
	return c == '_' || isalpha(c);
}

static u32 is_bin(u32 c)
{
	return c == '0' || c == '1';
}

static u32 is_oct(u32 c)
{
	return c >= '0' && c <= '7';
}

static u32 is_paren(u32 c)
{
	return c == '(' || c == ')';
}

static u32 is_bracket(u32 c)
{
	return c == '[' || c == ']';
}

static u32 is_brace(u32 c)
{
	return c == '{' || c == '}';
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

	strcpy(slash, "/");
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

static void path_updir(char *s)
{
	u32 c;
	char *p, *slash;
	for(p = s, slash = s; (c = *p); ++p)
	{
		if(c == '/')
		{
			slash = p;
		}
	}

	if(slash != s)
	{
		for(p = slash - 1; p > s; --p)
		{
			if(*p == '/')
			{
				break;
			}
		}
	}

	strcpy(p, "/");
}

static u32 last_char_is(const char *s, u32 c)
{
	u32 len = strlen(s);
	if(len == 0)
	{
		return 0;
	}

	return (u32)s[len - 1] == c;
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

static u32 count_char(const char *s, u32 val)
{
	u32 c;
	size_t cnt = 0;
	for(; (c = *s); ++s)
	{
		if(c == val)
		{
			++cnt;
		}
	}

	return cnt;
}

static i32 str_find(const char *haystack, u32 len, const char *needle, u32 sl)
{
	u32 i;
	if(len < sl)
	{
		return -1;
	}

	for(i = 0; i < len - sl; ++i)
	{
		if(!memcmp(haystack + i, needle, sl))
		{
			return i;
		}
	}

	return -1;
}

static char *_strdup(const char *s)
{
	size_t len = strlen(s) + 1;
	char *p = _malloc(len);
	memcpy(p, s, len);
	return p;
}

static u32 umin(u32 a, u32 b)
{
	return a < b ? a : b;
}

#if 0

static size_t revstrlen(const char *p)
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

#endif
