/* Common functions for "Load File" and "Save As" file browser */
static char _path_buf[PATH_MAX];
static char _fname_buf[PATH_MAX];
static tf _fld;

static dd _dd;
static char **_dir_list;
static u32 _dir_count;
static vec _filt_dir;

static void nav_init(void)
{
	tf_init(&_fld);
	get_working_dir(_path_buf);
	vec_init(&_filt_dir, 64 * sizeof(char *));
}

static void nav_destroy(void)
{
	_free(_dir_list);
	tf_destroy(&_fld);
	vec_destroy(&_filt_dir);
}

static void nav_title_render(char *s)
{
	char buf[PATH_MAX + 256];

	snprintf(buf, sizeof(buf), "%s: %s [%d]",
		s, _path_buf, _dir_count - 1);
	ed_render_line_str(buf, 0, 0, COLOR_BG, COLOR_FG);
}
