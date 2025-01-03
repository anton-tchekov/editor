/* Common functions for "Load File" and "Save As" file browser */
static vec _path_buf;
static vec _fname_buf;
static tf _fld;

static dd _dd;
static vec _dir_list;
static vec _filt_dir;

static void nav_init(void)
{
	tf_init(&_fld);
	_path_buf = get_working_dir();
	_filt_dir = vec_init(128 * sizeof(vec *));
}

static void nav_destroy(void)
{
	vec_destroy(&_dir_list);
	vec_destroy(&_filt_dir);
	vec_destroy(&_path_buf);
	vec_destroy(&_fname_buf);
	tf_destroy(&_fld);
}

static void nav_title_render(char *s)
{
	char buf[256];
	snprintf(buf, sizeof(buf), "%s: %s [%d]",
		s, vec_cstr(&_path_buf), vec_num_vecs(&_dir_list) - 1);
	ed_render_line_str(buf, 0, 0, COLOR_BG, COLOR_FG);
}
