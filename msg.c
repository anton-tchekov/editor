typedef enum
{
	MSG_INFO = 1,
	MSG_ERROR
} MessageType;

static u8 _msg_type;
static char _msg_buf[256];

static void msg_show(MessageType type, char *msg, ...)
{
	va_list args;
	_msg_type = type;
	va_start(args, msg);
	vsnprintf(_msg_buf, sizeof(_msg_buf), msg, args);
	va_end(args);
}

static u32 msg_render(void)
{
	u32 end = _screen_height;
	if(_msg_type)
	{
		--end;
		ed_render_line_str(_msg_buf, 0, end, screen_color(COLOR_TABLE_FG,
			_msg_type == MSG_INFO ? COLOR_TABLE_INFO : COLOR_TABLE_ERROR));
		_msg_type = 0;
	}

	return end;
}
