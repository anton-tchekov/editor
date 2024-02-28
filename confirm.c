static void mode_confirm(const char *msg, ...)
{
	va_list args;
	mode = ED_MODE_CONFIRM;
	va_start(args, msg);
	vsnprintf(confirm_buf, sizeof(confirm_buf), msg, args);
	va_end(args);
}

static void confirm_result(u32 yes)
{
	mode_default();
}

static void confirm_key_press(u32 key)
{
	switch(key)
	{
	case KEY_Z:
	case KEY_Y:
		confirm_result(1);
		break;

	case KEY_ESCAPE:
	case KEY_N:
		confirm_result(0);
		break;
	}
}
