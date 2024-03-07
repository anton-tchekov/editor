/* Go to line or symbol definition */
static char _goto_buf[16];
static field _goto_fld = { _goto_buf, sizeof(_goto_buf), 0, 0 };

static void mode_goto(void)
{
	mode = ED_MODE_GOTO;
	field_reset(&_goto_fld);
}

static void goto_return(void)
{
	u32 lnr;
	mode_default();
	field_add_nt(&_goto_fld);
	lnr = conv_lnr_str(_goto_fld.buf);
	if(lnr)
	{
		tb_gotoxy(tb, 0, lnr - 1);
		return;
	}

	tb_goto_def(tb, _goto_fld.buf);
}

static void goto_key_press(u32 key, u32 c)
{
	field_key(&_goto_fld, key, c);
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
	ed_render_nav(&_goto_fld, "Location: ");
	return 1;
}
