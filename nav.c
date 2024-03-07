/* Common functions for "Load File" and "Save As" file browser */

static char _path_buf[PATH_MAX];
static char _fld_buf[256];
static field _fld = { _fld_buf, sizeof(_fld_buf), 0, 0 };

static dropdown _dd;
static char **_dir_list;

static void nav_init(void)
{
	get_working_dir(_path_buf);
}

static void nav_cleanup(void)
{
	_free(_dir_list);
}

static void nav_dir_reload(void)
{
	dropdown_reset(&_dd);
	_free(_dir_list);
	_dir_list = dir_sorted(_path_buf, &_dd.count);
}

static void nav_title_render(char *s)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s: %s [%d]",
		s, _path_buf, _dd.count - 1);
	ed_render_line_str(buf, 0, 0, ptp(PT_BG, PT_FG));
}

static u32 nav_dir_render(u32 y)
{
	u32 i, end;
	end = umin(_dd.offset + DROPDOWN_PAGE, _dd.count);
	for(i = _dd.offset; i < end; ++i, ++y)
	{
		ed_render_line_str(_dir_list[i], 0, y, dropdown_color(&_dd, i));
	}

	return y;
}

static u32 nav_render(char *ac, char *in)
{
	nav_title_render(ac);
	field_render(&_fld, 1, 1, in);
	return nav_dir_render(2);
}
