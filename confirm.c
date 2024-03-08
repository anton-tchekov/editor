static char _confirm_buf[256];
static void (*_confirm_callback)(u32);

static void confirm(void (*callback)(u32), char *msg, ...)
{
	va_list args;
	_mode = MODE_CONFIRM;
	va_start(args, msg);
	vsnprintf(_confirm_buf, sizeof(_confirm_buf), msg, args);
	va_end(args);
	_confirm_callback = callback;
}

static void confirm_key_press(u32 key)
{
	switch(key)
	{
	case KEY_Z:
	case KEY_Y:
		_confirm_callback(1);
		break;

	case KEY_ESCAPE:
	case KEY_N:
		_confirm_callback(0);
		break;
	}
}

static u32 confirm_render(void)
{
	ed_render_line_str(_confirm_buf, 0, 0, ptp(PT_FG, PT_INFO));
	return 1;
}
