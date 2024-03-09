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

static u32 inc_wrap(u32 v, u32 max)
{
	if(++v == max)
	{
		v = 0;
	}

	return v;
}

static u32 dec_wrap(u32 v, u32 max)
{
	if(!v)
	{
		v = max;
	}

	return --v;
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

static u32 conv_lnr_str(char *s)
{
	u32 c;
	u32 lnr;

	for(lnr = 0; (c = *s); ++s)
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
	u32 cnt;

	cnt = 0;
	while(n)
	{
		++cnt;
		n /= 10;
	}

	return cnt;
}

static void linenr_str(char *s, u32 n, u32 width)
{
	char *p;

	p = s + width;
	*p-- = '\0';
	do
	{
		*p-- = n % 10 + '0';
		n /= 10;
	}
	while(n > 0);

	while(p >= s)
	{
		*p-- = ' ';
	}
}

static char *path_file(char *s)
{
	u32 c;
	char *slash;

	slash = s;
	for(; (c = *s); ++s)
	{
		if(c == '/')
		{
			slash = s + 1;
		}
	}

	return slash;
}

static char *get_file_ext(char *s)
{
	u32 c;
	char *p, *f;

	f = path_file(s);
	p = NULL;
	for(; (c = *f); ++f)
	{
		if(c == '.')
		{
			p = f + 1;
		}
	}

	if(!p)
	{
		p = f;
	}

	return p;
}

static void path_parent_dir(char *s)
{
	u32 c;
	char *p, *prev, *slash;

	prev = NULL;
	slash = NULL;
	for(p = s; (c = *p); ++p)
	{
		if(c == '/')
		{
			prev = slash;
			slash = p;
		}
	}

	if(prev)
	{
		prev[1] = '\0';
	}
}

static u32 last_char_is(char *s, u32 c)
{
	u32 len;

	len = strlen(s);
	if(len == 0)
	{
		return 0;
	}

	return (u32)s[len - 1] == c;
}

static u32 path_is_dir(char *s)
{
	return last_char_is(s, '/');
}

static u32 starts_with(char *str, char *prefix)
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

static u32 match_part(char *str, char *cmp, u32 len)
{
	return len == strlen(cmp) && !strncmp(str, cmp, len);
}

static u32 count_char(char *s, u32 val)
{
	u32 c, cnt;

	cnt = 0;
	for(; (c = *s); ++s)
	{
		if(c == val)
		{
			++cnt;
		}
	}

	return cnt;
}

static i32 str_find(char *haystack, u32 len, char *needle, u32 sl)
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

static char *_strdup(char *s)
{
	size_t len;
	char *p;

	len  = strlen(s) + 1;
	p = _malloc(len);
	memcpy(p, s, len);
	return p;
}

static u32 umin(u32 a, u32 b)
{
	return a < b ? a : b;
}

static u32 umax(u32 a, u32 b)
{
	return a > b ? a : b;
}
