/* TF (TextField): Single line text input field */
typedef struct
{
	vector v;
	u32 pos, sel;
} tf;

static void tf_clear(tf *t)
{
	vector_str_clear(&t->v);
	t->pos = 0;
	t->sel = 0;
}

static void tf_init(tf *t)
{
	vector_init(&t->v, 64);
	tf_clear(t);
}

static void tf_destroy(tf *t)
{
	vector_destroy(&t->v);
}

static void tf_sel_to_pos(tf *t)
{
	t->sel = t->pos;
}

static u32 tf_len(tf *t)
{
	return vector_len(&t->v) - 1;
}

static char *tf_str(tf *t)
{
	return vector_str(&t->v);
}

static u32 tf_sel_min(tf *t)
{
	return umin(t->sel, t->pos);
}

static u32 tf_sel_max(tf *t)
{
	return umax(t->sel, t->pos);
}

static void tf_sel_replace(tf *t, char *s, u32 len)
{
	u32 sel_start, sel_len;

	sel_start = tf_sel_min(t);
	sel_len = tf_sel_max(t) - sel_start;
	vector_replace(&t->v, sel_start, sel_len, s, len);
	t->pos = sel_start + len;
	tf_sel_to_pos(t);
}

static void tf_left(tf *t)
{
	if(t->sel == t->pos && t->pos > 0)
	{
		--t->pos;
	}

	tf_sel_to_pos(t);
}

static void tf_sel_left(tf *t)
{
	if(t->pos > 0)
	{
		--t->pos;
	}
}

static void tf_right(tf *t)
{
	if(t->sel == t->pos && t->pos < tf_len(t))
	{
		++t->pos;
	}

	tf_sel_to_pos(t);
}

static void tf_sel_right(tf *t)
{
	if(t->pos < tf_len(t))
	{
		++t->pos;
	}
}

static void tf_remove(tf *t)
{
	vector_remove(&t->v, t->pos, 1);
}

static void tf_backspace(tf *t)
{
	if(t->sel != t->pos)
	{
		tf_sel_replace(t, NULL, 0);
	}
	else if(t->pos > 0)
	{
		--t->pos;
		tf_sel_to_pos(t);
		tf_remove(t);
	}
}

static void tf_delete(tf *t)
{
	if(t->sel != t->pos)
	{
		tf_sel_replace(t, NULL, 0);
	}
	else if(t->pos < tf_len(t))
	{
		tf_remove(t);
	}
}

static void tf_sel_home(tf *t)
{
	t->pos = 0;
}

static void tf_home(tf *t)
{
	tf_sel_home(t);
	tf_sel_to_pos(t);
}

static void tf_sel_end(tf *t)
{
	t->pos = tf_len(t);
}

static void tf_end(tf *t)
{
	tf_sel_end(t);
	tf_sel_to_pos(t);
}

static void tf_char(tf *t, u32 chr)
{
	char ins[1];

	ins[0] = chr;
	tf_sel_replace(t, ins, 1);
}

static void tf_sel_all(tf *t)
{
	t->sel = 0;
	t->pos = tf_len(t);
}

static void tf_copy(tf *t)
{
	char *buf, *end;
	u32 c;

	buf = tf_str(t);
	end = buf + tf_sel_max(t);
	c = *end;
	*end = '\0';
	clipboard_store(buf + tf_sel_min(t));
	*end = c;
}

static void tf_cut(tf *t)
{
	tf_copy(t);
	tf_sel_replace(t, NULL, 0);
}

static void tf_paste(tf *t)
{
	char *s = clipboard_load();
	tf_sel_replace(t, s, strlen(s));
	free(s);
}

static u32 tf_key(tf *t, u32 key, u32 chr)
{
	switch(key)
	{
	case KEY_HOME:              tf_home(t);      break;
	case MOD_SHIFT | KEY_HOME:  tf_sel_home(t);  break;
	case KEY_END:               tf_end(t);       break;
	case MOD_SHIFT | KEY_END:   tf_sel_end(t);   break;
	case KEY_LEFT:              tf_left(t);      break;
	case MOD_SHIFT | KEY_LEFT:  tf_sel_left(t);  break;
	case KEY_RIGHT:             tf_right(t);     break;
	case MOD_SHIFT | KEY_RIGHT: tf_sel_right(t); break;
	case KEY_BACKSPACE:         tf_backspace(t); return 1;
	case KEY_DELETE:            tf_delete(t);    return 1;
	case MOD_CTRL | KEY_A:      tf_sel_all(t);   break;
	case MOD_CTRL | KEY_C:      tf_copy(t);      break;
	case MOD_CTRL | KEY_X:      tf_cut(t);       return 1;
	case MOD_CTRL | KEY_V:      tf_paste(t);     return 1;
	default:
		if(isprint(chr))
		{
			tf_char(t, chr);
			return 1;
		}
		break;
	}

	return 0;
}

static u32 tf_color(u32 a, u32 b, u32 i)
{
	return (i >= a && i < b) ? ptp(PT_FG, PT_GRAY) : ptp(PT_BG, PT_FG);
}

static u32 tf_color_focus(u32 a, u32 b, u32 i)
{
	return (i == b) ? ptp(PT_FG, PT_BG) :
		((i >= a && i < b) ? ptp(PT_FG, PT_INFO) : ptp(PT_BG, PT_FG));
}

static void tf_render(tf *t, u32 y, u32 focused, char *prompt)
{
	char *s;
	u32 x, a, b, i, c, len;

	for(s = prompt, x = 0; (c = *s); ++x, ++s)
	{
		screen_set(x, y, screen_pack(c, ptp(PT_BG, PT_FG)));
	}

	a = tf_sel_min(t);
	b = tf_sel_max(t);
	len = tf_len(t);
	s = tf_str(t);
	for(i = 0; x < _screen_width; ++x, ++s, ++i)
	{
		screen_set(x, y, screen_pack((i < len) ? *s : ' ',
			focused ? tf_color_focus(a, b, i) : tf_color(a, b, i)));
	}
}
