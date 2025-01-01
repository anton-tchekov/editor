/* CF (Confirm) */
static char _cf_buf[256];
static void (*_cf_callback)(u32);

static void cf_open(void (*callback)(u32), char *msg, ...)
{
	va_list args;
	_mode = MODE_CONFIRM;
	_cf_callback = callback;
	va_start(args, msg);
	vsnprintf(_cf_buf, sizeof(_cf_buf), msg, args);
	va_end(args);
}

static void cf_key(u32 key)
{
	switch(key)
	{
	case KEY_Z:
	case KEY_Y:
		_cf_callback(1);
		break;

	case KEY_ESCAPE:
	case KEY_N:
		_cf_callback(0);
		break;
	}
}

static u32 cf_render(void)
{
	ed_render_line_str(_cf_buf, 0, 0, COLOR_FG, COLOR_INFO);
	return 1;
}
