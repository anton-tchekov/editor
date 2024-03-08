/* Go to line or symbol definition */
static void mode_goto(void)
{
	_mode = MODE_GOTO;
	tf_clear(&_fld);
}

static void goto_return(void)
{
	u32 lnr;
	char *s;

	s = tf_str(&_fld);
	mode_default();
	lnr = conv_lnr_str(s);
	if(lnr)
	{
		tb_gotoxy(_tb, 0, lnr - 1);
		return;
	}

	tb_goto_def(_tb, s);
}

static void goto_key_press(u32 key, u32 c)
{
	tf_key(&_fld, key, c);
	key &= 0xFF;
	if(key == KEY_RETURN)
	{
		goto_return();
	}
	else if(key == KEY_ESCAPE)
	{
		mode_default();
	}
}

static u32 goto_render(void)
{
	tf_render(&_fld, 0, 1, "Location: ");
	return 1;
}
