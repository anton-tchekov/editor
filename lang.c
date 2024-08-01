enum
{
	LANGUAGE_UNKNOWN,
	LANGUAGE_C,
	LANGUAGE_DEFAULT = LANGUAGE_C,
	LANGUAGE_ASM6800,
	LANGUAGE_ASM65C02,
	LANGUAGE_COUNT
};

static char *lang_str(u32 lang)
{
	switch(lang)
	{
	case LANGUAGE_UNKNOWN:
		return "Text";
	case LANGUAGE_C:
		return "C";
	case LANGUAGE_ASM6800:
		return "6800 ASM";
	case LANGUAGE_ASM65C02:
		return "65C02 ASM";
	}

	return "???";
}
