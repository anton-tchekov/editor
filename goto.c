/* Go to line or symbol definition */
static void gt_open(void)
{
	_mode = MODE_GOTO;
	tf_clear(&_fld);
}

static void gt_key(u32 key, u32 c)
{
	tf_key(&_fld, key, c);
	key &= 0xFF;
	if(key == KEY_RETURN)
	{
		char *s;
		u32 lnr;

		s = tf_str(&_fld);
		lnr = conv_lnr_str(s);
		if(lnr)
		{
			tb_goto_xy(_tb, 0, lnr - 1);
		}
		else
		{
			tb_goto_def(_tb, s);
		}

		mode_default();
	}
	else if(key == KEY_ESCAPE)
	{
		mode_default();
	}
}

static u32 gt_render(void)
{
	tf_render(&_fld, 0, 1, "Location: ");
	return 1;
}
