/* Go to line or symbol definition */

static void mode_goto(void)
{
	mode = ED_MODE_GOTO;
	field_reset(&fld_goto);
}

static void goto_return(void)
{
	u32 lnr;
	mode_default();
	field_add_nt(&fld_goto);
	lnr = conv_lnr_str(fld_goto.buf);
	if(lnr)
	{
		tb_gotoxy(tb, 0, lnr - 1);
		return;
	}

	tb_goto_def(tb, fld_goto.buf);
}

static void goto_key_press(u32 key, u32 c)
{
	field_key(&fld_goto, key, c);
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
