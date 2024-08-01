typedef enum
{
	MSG_STATUS,
	MSG_INFO,
	MSG_ERROR
} MessageType;

static u64 _msg_open_ts;
static u8 _msg_type;
static char _msg_buf[256];

static void msg_show(MessageType type, char *msg, ...)
{
	va_list args;

	_msg_open_ts = get_ticks();
	_msg_type = type;
	va_start(args, msg);
	vsnprintf(_msg_buf, sizeof(_msg_buf), msg, args);
	va_end(args);
}

static u32 msg_color(u32 type)
{
	switch(type)
	{
	case MSG_INFO:
		return COLOR_INFO;

	case MSG_ERROR:
		return COLOR_ERROR;
	}

	return COLOR_STATUS;
}

static void msg_tryclose(void)
{
	if(_msg_type == MSG_STATUS)
	{
		return;
	}

	/* Close after 1 second */
	if(get_ticks() >= (_msg_open_ts + 1000 * 1000))
	{
		_msg_type = MSG_STATUS;
	}
}

static void msg_render(void)
{
	char *out = _msg_buf;

	if(_msg_type == MSG_STATUS)
	{
		/*if(tb)
		{
			snprintf(_msg_buf, sizeof(_msg_buf), "%s%s [%d Lines] [%d:%d - %d:%d] [%s]",
				tb->filename, tb->modified ? "*" : "", vec_len(&tb->lines),
				tb->sel.c[0].x,
				tb->sel.c[0].y,
				tb->sel.c[1].x,
				tb->sel.c[1].y,
				lang_str(tb->language));

			// return;
		}
		else*/
		{
			out = "Version 0.9";
		}
	}

	ed_render_line_str(out, 0, _full_height - 1, COLOR_FG,
		msg_color(_msg_type));

	msg_tryclose();
}
