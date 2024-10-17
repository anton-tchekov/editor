enum
{
	CHAR_BLOCK         =  0,
	CHAR_VISIBLE_SPACE =  1,
	CHAR_TAB_START     =  2,
	CHAR_TAB_MIDDLE    =  3,
	CHAR_TAB_END       =  4,
	CHAR_TAB_BOTH      =  5,
	CHAR_AE_LOWER      =  6,
	CHAR_AE_UPPER      =  7,
	CHAR_OE_LOWER      =  8,
	CHAR_OE_UPPER      = 11,
	CHAR_UE_LOWER      = 12,
	CHAR_UE_UPPER      = 13,
	CHAR_SZ            = 14,
	CHAR_PARAGRAPH     = 15,
	CHAR_DEGREE        = 16,
	CHAR_POW2          = 17,
	CHAR_POW3          = 18,
	CHAR_EURO          = 19,
	CHAR_MICRO         = 20
};

typedef struct
{
	u32 len;
	u32 chr;
	char utf8[4];
} ExtraChar;

static ExtraChar _extra_chars[] =
{
	{ 0, CHAR_AE_LOWER,  "\xC3\xA4" },
	{ 0, CHAR_AE_UPPER,  "\xC3\x84" },
	{ 0, CHAR_OE_LOWER,  "\xC3\xB6" },
	{ 0, CHAR_OE_UPPER,  "\xC3\x96" },
	{ 0, CHAR_UE_LOWER,  "\xC3\xBC" },
	{ 0, CHAR_UE_UPPER,  "\xC3\x9C" },
	{ 0, CHAR_SZ,        "\xC3\x9F" },
	{ 0, CHAR_PARAGRAPH, "\xC2\xA7" },
	{ 0, CHAR_DEGREE,    "\xC2\xB0" },
	{ 0, CHAR_POW2,      "\xC2\xB2" },
	{ 0, CHAR_POW3,      "\xC2\xB3" },
	{ 0, CHAR_EURO,      "\xE2\x82\xAC" },
	{ 0, CHAR_MICRO,     "\xC2\xB5" }
};

typedef struct
{
	u32 len;
	char *utf8;
} LutChar;

static LutChar utf8_lut[32];

static void utf8_lut_init(void)
{
	for(u32 i = 0; i < ARRLEN(_extra_chars); ++i)
	{
		ExtraChar *ec = _extra_chars + i;
		LutChar *lp = utf8_lut + ec->chr;
		ec->len = lp->len = strlen(ec->utf8);
		lp->utf8 = ec->utf8;
	}
}

static char *convert_to_utf8(char *s, u32 *len)
{
	vec fixed;
	vec_init(&fixed, *len + 1);
	char *e = s + *len;
	for(; s < e; ++s)
	{
		u32 c = *s;
		if(isprint(c) || c == '\t' || c == '\n')
		{
			vec_pushbyte(&fixed, c);
		}
		else
		{
			vec_push(&fixed, utf8_lut[c].len, utf8_lut[c].utf8);
		}
	}

	*len = vec_len(&fixed);
	vec_pushbyte(&fixed, '\0');
	return vec_data(&fixed);
}

static u32 utf8_find(char *in, u32 *len)
{
	for(u32 i = 0; i < ARRLEN(_extra_chars); ++i)
	{
		ExtraChar *ec = _extra_chars + i;
		if(ec->len > *len) { continue; }
		if(!memcmp(in, ec->utf8, ec->len))
		{
			*len = ec->len;
			return ec->chr;
		}
	}

	return 0;
}

static char *convert_from_utf8(char *s, u32 *len)
{
	vec out;
	char *e = s + *len;
	vec_init(&out, *len);
	while(s < e)
	{
		u32 c = *s;
		if(isprint(c) || c == '\n' || c == '\t')
		{
			vec_pushbyte(&out, c);
			++s;
		}
		else
		{
			u32 remain = e - s;
			if(!(c = utf8_find(s, &remain)))
			{
				vec_destroy(&out);
				return NULL;
			}

			vec_pushbyte(&out, c);
			s += remain;
		}
	}

	*len = out.len;
	vec_pushbyte(&out, '\0');
	return out.data;
}
