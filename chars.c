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

struct
{
	u32 chr;
	char utf8[4];
}
_extra_chars[] =
{
	{ CHAR_AE_LOWER,  "\xC3\xA4" },
	{ CHAR_AE_UPPER,  "\xC3\x84" },
	{ CHAR_OE_LOWER,  "\xC3\xB6" },
	{ CHAR_OE_UPPER,  "\xC3\x96" },
	{ CHAR_UE_LOWER,  "\xC3\xBC" },
	{ CHAR_UE_UPPER,  "\xC3\x9C" },
	{ CHAR_SZ,        "\xC3\x9F" },
	{ CHAR_PARAGRAPH, "\xC2\xA7" },
	{ CHAR_DEGREE,    "\xC2\xB0" },
	{ CHAR_POW2,      "\xC2\xB2" },
	{ CHAR_POW3,      "\xC2\xB3" },
	{ CHAR_EURO,      "\xE2\x82\xAC" },
	{ CHAR_MICRO,     "\xC2\xB5" }
};
