/* Go to line or symbol definition */
static void mode_goto(void)
{
	_mode = ED_MODE_GOTO;
	field_reset(&_fld);
}

static void goto_return(void)
{
	u32 lnr;
	mode_default();
	field_add_nt(&_fld);
	lnr = conv_lnr_str(_fld.buf);
	if(lnr)
	{
		tb_gotoxy(_tb, 0, lnr - 1);
		return;
	}

	tb_goto_def(_tb, _fld.buf);
}

static void goto_key_press(u32 key, u32 c)
{
	field_key(&_fld, key, c);
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
	field_render(&_fld, 0, 1, "Location: ");
	return 1;
}
