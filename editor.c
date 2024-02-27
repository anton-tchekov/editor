#include "util.c"
#include "keyword.c"
#include "test.c"

enum
{
	ED_MODE_DEFAULT,
	ED_MODE_NAV,
	ED_MODE_OPENED,
	ED_MODE_COUNT
};

enum
{
	ED_INFO,
	ED_ERROR
};

enum
{
	LANGUAGE_UNKNOWN,
	LANGUAGE_C,
	LANGUAGE_DEFAULT = LANGUAGE_C,
	LANGUAGE_ASM6800,
	LANGUAGE_COUNT
};

#define MAX_SEARCH_LEN    256
#define MAX_MSG_LEN        80
#define ED_DIR_PAGE        12
#define COMMENT_LOOKBACK   10

#include "cursor.c"

static u32 offset_x, page_w;

static u8 msg_show;
static u8 mode;
static u32 tabsize;
static u8 show_linenr;
static u8 show_whitespace;

#include "textbuf.c"

static u8 msg_type;
static char msg_buf[MAX_MSG_LEN];

static u32 nav_cursor, nav_len;
static char *nav_buf, nav_base[MAX_SEARCH_LEN];

static u8 first_compare;
static char *search_file, same[MAX_SEARCH_LEN];

static char **dir_list;
static u32 dir_entries, dir_offset, dir_pos;

static vector buffers;
static u32 cur_buf;
static u32 untitled_cnt = 1;
static textbuf *tb;

#include "buffers.c"
#include "render.c"

static void ed_mode_default(void)
{
	mode = ED_MODE_DEFAULT;
}

static void ed_msg(u32 type, const char *msg, ...)
{
	va_list args;
	msg_show = 1;
	msg_type = type;
	va_start(args, msg);
	vsnprintf(msg_buf, MAX_MSG_LEN, msg, args);
	va_end(args);
}

static void ed_load(const char *filename)
{
	textbuf *t;
	char *buf;
	if(bf_switch_name(filename))
	{
		return;
	}

	switch(textfile_read(filename, &buf))
	{
	case FILE_READ_FAIL:
		ed_msg(ED_ERROR, "Failed to open file");
		return;

	case FILE_READ_NOT_TEXT:
		ed_msg(ED_ERROR, "Invalid character, binary file?");
		return;
	}

	t = tb_new(filename, buf, 1, LANGUAGE_DEFAULT);
	bf_insert_cur(t);
	_free(buf);
	ed_mode_default();
}

static void ed_new(void)
{
	textbuf *t;
	char name[32];
	snprintf(name, sizeof(name), "untitled-%d", untitled_cnt++);
	t = tb_new(name, NULL, 0, LANGUAGE_DEFAULT);
	bf_insert_cur(t);
	ed_mode_default();
}

static void ed_init(void)
{
	bf_init();
	ed_new();
	tabsize = 4;
	show_whitespace = 1;
	show_linenr = 1;
	nav_buf = get_working_dir(nav_base);
}

static void ed_toggle_line_nr(void)
{
	show_linenr = !show_linenr;
}

static void ed_tab_size(void)
{
	tabsize <<= 1;
	if(tabsize > 8)
	{
		tabsize = 2;
	}
}

static void ed_whitespace(void)
{
	show_whitespace = !show_whitespace;
}

static void ed_save_as(void)
{
}

static void ed_save(void)
{
	if(!tb->modified)
	{
		return;
	}

	if(tb->exists)
	{
		u32 len;
		char *buf = tb_export(tb, &len);
		if(file_write(tb->filename, buf, len))
		{
			ed_msg(ED_ERROR, "Writing file failed");
		}
		else
		{
			ed_msg(ED_INFO, "File saved");
			tb->modified = 0;
		}
		_free(buf);
	}
	else
	{
		ed_save_as();
	}
}

static void ed_cleanup(void)
{
	bf_destroy();
	_free(dir_list);
}

#include "nav.c"
#include "opened.c"

static void ed_quit(void)
{
	if(bf_has_modified())
	{
		ed_mode_opened();
		return;
	}

	ed_cleanup();
	request_exit();
}

static void event_scroll(i32 y)
{
	tb_scroll(tb, -3 * y);
	ed_render();
}

static void event_dblclick(u32 x, u32 y)
{
	tb_double_click(tb, x, y);
	ed_render();
}

static void event_tripleclick(u32 x, u32 y)
{
	tb_triple_click(tb, x, y);
	ed_render();
}

static void event_mousedown(u32 x, u32 y)
{
	tb_mouse_cursor(tb, x, y);
	ed_render();
}

static void event_mousemove(u32 x, u32 y)
{
	tb_mouse_sel(tb, x, y);
	ed_render();
}

static void ed_key_press_default(u32 key, u32 cp)
{
	if(!tb)
	{
		switch(key)
		{
		case MOD_CTRL | KEY_G:
		case MOD_CTRL | KEY_O:  ed_open();        break;
		case MOD_CTRL | KEY_T:
		case MOD_CTRL | KEY_N:  ed_new();         break;
		case MOD_CTRL | KEY_Q:  ed_quit();        break;
		case MOD_CTRL | KEY_B:  ed_mode_opened(); break;
		}
		return;
	}

	switch(key)
	{
	case MOD_CTRL | MOD_SHIFT | KEY_LEFT:   tb_sel_prev_word(tb);   break;
	case MOD_CTRL | KEY_LEFT:               tb_prev_word(tb);       break;
	case MOD_SHIFT | KEY_LEFT:              tb_sel_left(tb);        break;
	case KEY_LEFT:                          tb_left(tb);            break;
	case MOD_CTRL | MOD_SHIFT | KEY_RIGHT:  tb_sel_next_word(tb);   break;
	case MOD_CTRL | KEY_RIGHT:              tb_next_word(tb);       break;
	case MOD_SHIFT | KEY_RIGHT:             tb_sel_right(tb);       break;
	case KEY_RIGHT:                         tb_right(tb);           break;
	case MOD_CTRL | KEY_UP:                 tb_move_up(tb);         break;
	case MOD_SHIFT | KEY_UP:                tb_sel_up(tb);          break;
	case KEY_UP:                            tb_up(tb);              break;
	case MOD_CTRL | KEY_DOWN:               tb_move_down(tb);       break;
	case MOD_SHIFT | KEY_DOWN:              tb_sel_down(tb);        break;
	case KEY_DOWN:                          tb_down(tb);            break;
	case MOD_SHIFT | KEY_PAGE_UP:           tb_sel_page_up(tb);     break;
	case KEY_PAGE_UP:                       tb_page_up(tb);         break;
	case MOD_SHIFT | KEY_PAGE_DOWN:         tb_sel_page_down(tb);   break;
	case KEY_PAGE_DOWN:                     tb_page_down(tb);       break;
	case MOD_CTRL | MOD_SHIFT | KEY_HOME:   tb_sel_top(tb);         break;
	case MOD_CTRL | KEY_HOME:               tb_top(tb);             break;
	case MOD_SHIFT | KEY_HOME:              tb_sel_home(tb);        break;
	case KEY_HOME:                          tb_home(tb);            break;
	case MOD_CTRL | MOD_SHIFT | KEY_END:    tb_sel_bottom(tb);      break;
	case MOD_CTRL | KEY_END:                tb_bottom(tb);          break;
	case MOD_SHIFT | KEY_END:               tb_sel_end(tb);         break;
	case KEY_END:                           tb_end(tb);             break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                        tb_enter(tb);           break;
	case MOD_CTRL | KEY_RETURN:             tb_enter_after(tb);     break;
	case MOD_CTRL | MOD_SHIFT | KEY_RETURN: tb_enter_before(tb);    break;
	case MOD_CTRL | KEY_BACKSPACE:          tb_del_prev_word(tb);   break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:                     tb_backspace(tb);       break;
	case MOD_CTRL | KEY_DELETE:             tb_del_next_word(tb);   break;
	case MOD_SHIFT | KEY_DELETE:            tb_del_cur_line(tb);    break;
	case KEY_DELETE:                        tb_delete(tb);          break;
	case MOD_CTRL | KEY_L:                  tb_sel_cur_line(tb);    break;
	case MOD_CTRL | MOD_SHIFT | KEY_L:      ed_toggle_line_nr();    break;
	case MOD_CTRL | KEY_K:                  tb_toggle_lang(tb);     break;
	case MOD_CTRL | KEY_G:                  ed_goto();              break;
	case MOD_CTRL | KEY_O:                  ed_open();              break;
	case MOD_CTRL | KEY_C:                  tb_copy(tb);            break;
	case MOD_CTRL | KEY_X:                  tb_cut(tb);             break;
	case MOD_CTRL | KEY_V:                  tb_paste(tb);           break;
	case MOD_CTRL | KEY_S:                  ed_save();              break;
	case MOD_CTRL | MOD_SHIFT | KEY_S:      ed_save_as();           break;
	case MOD_CTRL | MOD_SHIFT | KEY_T:      ed_tab_size();          break;
	case MOD_CTRL | KEY_T:
	case MOD_CTRL | KEY_N:                  ed_new();               break;
	case MOD_CTRL | KEY_W:                  bf_discard_cur();       break;
	case MOD_CTRL | KEY_Q:                  ed_quit();              break;
	case MOD_CTRL | KEY_J:                  ed_whitespace();        break;
	case MOD_CTRL | KEY_D:                  tb_trailing(tb);        break;
	case MOD_CTRL | KEY_B:                  ed_mode_opened();       break;
	case MOD_CTRL | KEY_I:                  tb_ins_include(tb);     break;
	case MOD_CTRL | MOD_SHIFT | KEY_I:      tb_ins_include_lib(tb); break;
	case MOD_CTRL | MOD_SHIFT | KEY_A:      tb_ins_comment(tb);     break;
	case MOD_CTRL | KEY_A:                  tb_sel_all(tb);         break;
	case MOD_CTRL | KEY_TAB:                bf_cycle();             break;
	default:
		if(isprint(cp) || cp == '\t')
		{
			tb_char(tb, cp);
		}
		break;
	}
}

static void ed_key_press(u32 key, u32 chr)
{
	switch(mode)
	{
	case ED_MODE_DEFAULT:
		ed_key_press_default(key, chr);
		break;

	case ED_MODE_NAV:
		ed_key_press_nav(key, chr);
		break;

	case ED_MODE_OPENED:
		ed_key_press_opened(key);
		break;
	}

	ed_render();
}

static void event_keyboard(u32 key, u32 cp, u32 state)
{
	if(state == KEYSTATE_RELEASED)
	{
		return;
	}

	ed_key_press(key, cp);
}

static void event_resize(void)
{
	ed_render();
}

static void event_init(int argc, char *argv[])
{
#ifndef NDEBUG
	test_run_all();
#endif

	keyword_init(&c_hashmap);
	keyword_init(&asm_hashmap);
	ed_init();

	if(argc == 2)
	{
		ed_load(argv[1]);
	}

	ed_render();
}

static u32 event_exit(void)
{
	if(bf_has_modified())
	{
		ed_mode_opened();
		ed_render();
		return 0;
	}

	ed_cleanup();
	return 1;
}
