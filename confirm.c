static void mode_confirm(void (*callback)(u32), const char *msg, ...)
{
	va_list args;
	mode = ED_MODE_CONFIRM;
	va_start(args, msg);
	vsnprintf(confirm_buf, sizeof(confirm_buf), msg, args);
	va_end(args);
	confirm_result = callback;
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
