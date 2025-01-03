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

static u32 is_1_to_9(u32 c)
{
	return c >= '1' && c <= '9';
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
	u32 lnr = 0;
	for(u32 c; (c = *s); ++s)
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

static void linenr_str(char *s, u32 n, u32 width)
{
	char *p = s + width;
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
	char *f = path_file(s);
	char *p = NULL;
	for(u32 c; (c = *f); ++f)
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

static void path_parent_dir(vec *v)
{
	char *s = vec_str(v);
	char *prev = NULL;
	char *slash = NULL;
	char *p = s;
	for(u32 c; (c = *p); ++p)
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
	u32 len = strlen(s);
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

static u32 starts_with(vec *str, vec *prefix)
{
	return (prefix->len <= str->len &&
		!memcmp(str->data, prefix->data, prefix->len));
}

static u32 match_part(char *str, char *cmp, u32 len)
{
	return len == strlen(cmp) && !strncmp(str, cmp, len);
}

static u32 str_equals_len(char *s1, u32 l1, char *s2, u32 l2)
{
	return l1 == l2 && !memcmp(s1, s2, l1);
}

static u32 count_char(char *s, u32 val)
{
	u32 c, cnt = 0;
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
	if(len < sl)
	{
		return -1;
	}

	for(u32 i = 0; i < len - sl; ++i)
	{
		if(!memcmp(haystack + i, needle, sl))
		{
			return i;
		}
	}

	return -1;
}

static i32 escape_seq(char *out, char *s)
{
	u32 c;
	char *p;

	for(p = out; (c = *s); ++s)
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
			default: return -1;
			}
		}

		*p++ = c;
	}

	*p = '\0';
	return p - out;
}

static void uppercase(char *s, u32 len)
{
	for(char *end = s + len; s < end; ++s) { *s = toupper(*s); }
}

static void lowercase(char *s, u32 len)
{
	for(char *end = s + len; s < end; ++s) { *s = tolower(*s); }
}

static void camelcase(char *s, u32 len, char *out)
{

}

static void pascalcase(char *s, u32 len, char *out)
{
	
}

static void kebabcase(char *s, u32 len)
{
	
}

static void snakecase(char *s, u32 len)
{
/*
public static string ToSnakeCase(this string text)
{
	if (string.IsNullOrEmpty(text))
	{
		return text;
	}

	var builder = new StringBuilder(text.Length + Math.Min(2, text.Length / 5));
	var previousCategory = default(UnicodeCategory?);

	for (var currentIndex = 0; currentIndex < text.Length; currentIndex++)
	{
		var currentChar = text[currentIndex];
		if (currentChar == '_')
		{
			builder.Append('_');
			previousCategory = null;
			continue;
		}

		var currentCategory = char.GetUnicodeCategory(currentChar);
		switch (currentCategory)
		{
			case UnicodeCategory.UppercaseLetter:
			case UnicodeCategory.TitlecaseLetter:
				if (previousCategory == UnicodeCategory.SpaceSeparator ||
					previousCategory == UnicodeCategory.LowercaseLetter ||
					previousCategory != UnicodeCategory.DecimalDigitNumber &&
					previousCategory != null &&
					currentIndex > 0 &&
					currentIndex + 1 < text.Length &&
					char.IsLower(text[currentIndex + 1]))
				{
					builder.Append('_');
				}

				currentChar = char.ToLower(currentChar, CultureInfo.InvariantCulture);
				break;

			case UnicodeCategory.LowercaseLetter:
			case UnicodeCategory.DecimalDigitNumber:
				if (previousCategory == UnicodeCategory.SpaceSeparator)
				{
					builder.Append('_');
				}
				break;

			default:
				if (previousCategory != null)
				{
					previousCategory = UnicodeCategory.SpaceSeparator;
				}
				continue;
		}

		builder.Append(currentChar);
		previousCategory = currentCategory;
	}

	return builder.ToString();
}
*/
}
