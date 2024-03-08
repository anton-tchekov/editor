/* Common functions for "Load File" and "Save As" file browser */
static char _path_buf[PATH_MAX];
static char _fld_buf[256];
static field _fld = { _fld_buf, sizeof(_fld_buf), 0, 0 };

static dropdown _dd;
static char **_dir_list;
static u32 _dir_count;
static vector _filt_dir;

static void nav_init(void)
{
	get_working_dir(_path_buf);
	vector_init(&_filt_dir, 64 * sizeof(char *));
}

static void nav_cleanup(void)
{
	_free(_dir_list);
	vector_destroy(&_filt_dir);
}

static void nav_title_render(char *s)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s: %s [%d]",
		s, _path_buf, _dir_count - 1);
	ed_render_line_str(buf, 0, 0, ptp(PT_BG, PT_FG));
}
