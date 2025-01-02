/* TF (TextField): Single line text input field */
typedef struct
{
	vec v;
	u32 pos, sel;
} tf;

static void tf_clear(tf *t)
{
	vec_str_clear(&t->v);
	t->pos = 0;
	t->sel = 0;
}

static void tf_init(tf *t)
{
	t->v = vec_init(64);
	tf_clear(t);
}

static void tf_sel_to_pos(tf *t)
{
	t->sel = t->pos;
}

static void tf_set(tf *t, char *s, u32 len)
{
	vec_clear(&t->v);
	vec_push(&t->v, len, s);
	vec_push(&t->v, 1, "");
	t->pos = len;
	tf_sel_to_pos(t);
}

static void tf_destroy(tf *t)
{
	vec_destroy(&t->v);
}

static u32 tf_bufsiz(tf *t)
{
	return vec_len(&t->v);
}

static u32 tf_len(tf *t)
{
	return tf_bufsiz(t) - 1;
}

static char *tf_str(tf *t)
{
	return vec_str(&t->v);
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
	vec_replace(&t->v, sel_start, sel_len, s, len);
	t->pos = sel_start + len;
	tf_sel_to_pos(t);
}

static void tf_left(tf *t)
{
	if(t->sel != t->pos)
	{
		t->pos = tf_sel_min(t);
	}
	else if(t->pos > 0)
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
	if(t->sel != t->pos)
	{
		t->pos = tf_sel_max(t);
	}
	else if(t->pos < tf_len(t))
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
	vec_remove(&t->v, t->pos, 1);
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
	buf = tf_str(t);
	end = buf + tf_sel_max(t);
	u32 c = *end;
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

static void tf_render(tf *t, u32 y, u32 focused, char *prompt)
{
	char *s = prompt;
	u32 x = 0;
	for(u32 c; (c = *s); ++x, ++s)
	{
		render_char(x, y, c, COLOR_BG, COLOR_FG);
	}

	u32 a = tf_sel_min(t);
	u32 b = tf_sel_max(t);
	u32 len = tf_len(t);
	s = tf_str(t);
	for(u32 i = 0; x < _screen_width; ++x, ++s, ++i)
	{
		u32 fg = COLOR_BG;
		u32 bg = COLOR_FG;
		if(focused)
		{
			if(i == t->pos)
			{
				fg = COLOR_FG;
				bg = COLOR_BG;
			}
			else if(i >= a && i < b)
			{
				fg = COLOR_FG;
				bg = COLOR_INFO;
			}
		}
		else if(i >= a && i < b)
		{
			fg = COLOR_FG;
			bg = COLOR_GRAY;
		}

		render_char(x, y, (i < len) ? *s : ' ', fg, bg);
	}
}
