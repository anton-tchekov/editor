typedef enum
{
	CT_SPACE,
	CT_IDENTIFIER,
	CT_OTHER
} CharType;

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

static CharType char_type(u32 c)
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

static u32 starts_with(const char *str, const char *prefix)
{
	u32 c, p;
	while((p = *prefix++) && (c = *str++))
	{
		if(c != p)
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
