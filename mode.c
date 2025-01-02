enum
{
	MODE_DEFAULT,
	MODE_OPEN,
	MODE_GOTO,
	MODE_SAVE_AS,
	MODE_OPENED,
	MODE_CONFIRM,
	MODE_SEARCH,
	MODE_CMD,
	MODE_COUNT
};

static u8 _mode;

static void mode_default(void)
{
	_mode = MODE_DEFAULT;
}
