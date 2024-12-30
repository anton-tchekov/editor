#include "util.c"
#include "keyword.c"
#include "test.c"

enum
{
	MODE_DEFAULT,
	MODE_OPEN,
	MODE_GOTO,
	MODE_SAVE_AS,
	MODE_OPENED,
	MODE_CONFIRM,
	MODE_SEARCH,
	MODE_CMD,
	MODE_COUNT
};

#include "lang.c"

#define DD_PAGE          12
#define COMMENT_LOOKBACK 10

static u32 _offset_x, _page_w;

static u8
	_mode,
	_tabsize,
	_show_linenr,
	_show_whitespace;

static void mode_default(void)
{
	_mode = MODE_DEFAULT;
}

static void ed_render_line_str(char *s, u32 x, u32 y, u32 fg, u32 bg)
{
	for(; *s && x < _screen_width; ++s, ++x)
	{
		render_char(x, y, *s, fg, bg);
	}

	for(; x < _screen_width; ++x)
	{
		render_char(x, y, ' ', fg, bg);
	}
}

#include "cursor.c"
#include "msg.c"
#include "textbuf.c"
#include "buffers.c"
#include "render.c"
#include "confirm.c"
#include "cmd.c"

static u32 ed_detect_language(char *filename)
{
	char *ext;

	ext = get_file_ext(filename);
	if(!strcmp(ext, "c") || !strcmp(ext, "cpp") ||
		!strcmp(ext, "h") || !strcmp(ext, "hpp"))
	{
		return LANGUAGE_C;
	}

	if(!strcmp(ext, "s") || !strcmp(ext, "S"))
	{
		return LANGUAGE_ASM6800;
	}

	return LANGUAGE_UNKNOWN;
}

static void ed_load(char *filename)
{
	if(bf_switch_name(filename))
	{
		mode_default();
		return;
	}

	u32 len;
	char *buf;
	switch(textfile_read(filename, &buf, &len))
	{
	case FILE_READ_FAIL:
		msg_show(MSG_ERROR, "Failed to open file");
		return;

	case FILE_READ_NOT_TEXT:
		msg_show(MSG_ERROR, "Invalid character, binary file?");
		return;
	}

	char *conv = convert_from_utf8(buf, &len);
	_free(buf);
	if(!conv)
	{
		msg_show(MSG_ERROR, "Unsupported UTF8 character");
		return;
	}

	textbuf *t = tb_new(filename, conv, 1, ed_detect_language(filename));
	bf_insert_cur(t);
	_free(conv);
	mode_default();
}

static void ed_new(void)
{
	char name[32];
	snprintf(name, sizeof(name), "untitled-%d", _untitled_cnt++);
	textbuf *t = tb_new(name, NULL, 0, LANGUAGE_DEFAULT);
	bf_insert_cur(t);
	mode_default();
}

static void ed_toggle_line_nr(void)
{
	_show_linenr = !_show_linenr;
}

static void ed_tab_size(void)
{
	_tabsize <<= 1;
	if(_tabsize > 8)
	{
		_tabsize = 2;
	}
}

static void ed_whitespace(void)
{
	_show_whitespace = !_show_whitespace;
}

#include "dropdown.c"
#include "textfld.c"
#include "nav.c"
#include "goto.c"
#include "open.c"
#include "save_as.c"
#include "search.c"

static void ed_init(void)
{
	bf_init();
	sr_init();
	utf8_lut_init();
	kw_init(&_kw_c);
	kw_init(&_kw_asm_6800);
	kw_init(&_kw_asm_65C02);
	_tabsize = 4;
	_show_whitespace = 1;
	_show_linenr = 1;
	nav_init();
}

static void ed_destroy(void)
{
	sr_destroy();
	bf_destroy();
	nav_destroy();
}

static void ed_save(void)
{
	if(!_tb->modified)
	{
		return;
	}

	if(_tb->exists)
	{
		u32 len;
		char *buf = tb_export(_tb, &len);
		char *utf8 = convert_to_utf8(buf, &len);
		_free(buf);
		if(file_write(_tb->filename, utf8, len))
		{
			msg_show(MSG_ERROR, "Writing file failed");
		}
		else
		{
			msg_show(MSG_INFO, "File saved");
			_tb->modified = 0;
		}

		_free(utf8);
	}
	else
	{
		mode_save_as();
	}
}

#include "opened.c"

static void ed_render(void)
{
	u32 start_y = 0;
	switch(_mode)
	{
	case MODE_OPEN:
		start_y = op_render();
		break;

	case MODE_GOTO:
		start_y = gt_render();
		break;

	case MODE_SAVE_AS:
		start_y = sv_render();
		break;

	case MODE_OPENED:
		start_y = ob_render();
		break;

	case MODE_CONFIRM:
		start_y = cf_render();
		break;

	case MODE_SEARCH:
		start_y = sr_render();
		break;
	}

	msg_render();
	u32 end_y = _screen_height;
	if(_tb)
	{
		ed_render_buffer(start_y, end_y);
	}
	else
	{
		ed_render_blank(start_y, end_y);
	}
}

static void ed_quit(void)
{
	if(bf_has_modified())
	{
		ob_open();
		return;
	}

	ed_destroy();
	request_exit();
}

static void default_key(u32 key, u32 cp)
{
	if(!_tb)
	{
		switch(key)
		{
		case MOD_CTRL | KEY_O:  mode_open();   break;
		case MOD_CTRL | KEY_N:  ed_new();      break;
		case MOD_CTRL | KEY_Q:  ed_quit();     break;
		case MOD_CTRL | KEY_B:
		case MOD_CTRL | KEY_P:  ob_open();     break;
		}
		return;
	}

	switch(key)
	{
	case MOD_CTRL | MOD_SHIFT | KEY_LEFT:   tb_sel_prev_word(_tb);   break;
	case MOD_CTRL | KEY_LEFT:               tb_prev_word(_tb);       break;
	case MOD_SHIFT | KEY_LEFT:              tb_sel_left(_tb);        break;
	case KEY_LEFT:                          tb_left(_tb);            break;
	case MOD_CTRL | MOD_SHIFT | KEY_RIGHT:  tb_sel_next_word(_tb);   break;
	case MOD_CTRL | KEY_RIGHT:              tb_next_word(_tb);       break;
	case MOD_SHIFT | KEY_RIGHT:             tb_sel_right(_tb);       break;
	case KEY_RIGHT:                         tb_right(_tb);           break;
	case MOD_CTRL | KEY_UP:                 tb_move_up(_tb);         break;
	case MOD_SHIFT | KEY_UP:                tb_sel_up(_tb);          break;
	case KEY_UP:                            tb_up(_tb);              break;
	case MOD_CTRL | KEY_DOWN:               tb_move_down(_tb);       break;
	case MOD_SHIFT | KEY_DOWN:              tb_sel_down(_tb);        break;
	case KEY_DOWN:                          tb_down(_tb);            break;
	case MOD_SHIFT | KEY_PAGE_UP:           tb_sel_page_up(_tb);     break;
	case KEY_PAGE_UP:                       tb_page_up(_tb);         break;
	case MOD_SHIFT | KEY_PAGE_DOWN:         tb_sel_page_down(_tb);   break;
	case KEY_PAGE_DOWN:                     tb_page_down(_tb);       break;
	case MOD_CTRL | MOD_SHIFT | KEY_HOME:   tb_sel_top(_tb);         break;
	case MOD_CTRL | KEY_HOME:               tb_top(_tb);             break;
	case MOD_SHIFT | KEY_HOME:              tb_sel_home(_tb);        break;
	case KEY_HOME:                          tb_home(_tb);            break;
	case MOD_CTRL | MOD_SHIFT | KEY_END:    tb_sel_bottom(_tb);      break;
	case MOD_CTRL | KEY_END:                tb_bottom(_tb);          break;
	case MOD_SHIFT | KEY_END:               tb_sel_end(_tb);         break;
	case KEY_END:                           tb_end(_tb);             break;
	case MOD_SHIFT | KEY_RETURN:
	case KEY_RETURN:                        tb_enter(_tb);           break;
	case MOD_CTRL | KEY_RETURN:             tb_enter_after(_tb);     break;
	case MOD_CTRL | MOD_SHIFT | KEY_RETURN: tb_enter_before(_tb);    break;
	case MOD_CTRL | KEY_BACKSPACE:          tb_del_prev_word(_tb);   break;
	case MOD_SHIFT | KEY_BACKSPACE:
	case KEY_BACKSPACE:                     tb_backspace(_tb);       break;
	case MOD_CTRL | KEY_DELETE:             tb_del_next_word(_tb);   break;
	case MOD_SHIFT | KEY_DELETE:            tb_del_cur_line(_tb);    break;
	case KEY_DELETE:                        tb_delete(_tb);          break;
	case KEY_INSERT:                        tb_toggle_insert(_tb);   break;
	case MOD_CTRL | KEY_L:                  tb_sel_cur_line(_tb);    break;
	case MOD_CTRL | MOD_SHIFT | KEY_L:      ed_toggle_line_nr();     break;
	case MOD_CTRL | KEY_K:                  tb_toggle_lang(_tb);     break;
	case MOD_CTRL | KEY_G:                  gt_open();               break;
	case MOD_CTRL | KEY_O:                  mode_open();             break;
	case MOD_CTRL | KEY_C:                  tb_copy(_tb);            break;
	case MOD_CTRL | KEY_X:                  tb_cut(_tb);             break;
	case MOD_CTRL | KEY_V:                  tb_paste(_tb);           break;
	case MOD_CTRL | KEY_S:                  ed_save();               break;
	case MOD_CTRL | MOD_SHIFT | KEY_S:      mode_save_as();          break;
	case MOD_CTRL | MOD_SHIFT | KEY_T:      ed_tab_size();           break;
	case MOD_CTRL | KEY_N:                  ed_new();                break;
	case MOD_CTRL | KEY_W:                  bf_discard_cur();        break;
	case MOD_CTRL | KEY_Q:                  ed_quit();               break;
	case MOD_CTRL | KEY_J:                  ed_whitespace();         break;
	case MOD_CTRL | KEY_D:                  tb_trailing(_tb);        break;
	case MOD_CTRL | KEY_B:
	case MOD_CTRL | KEY_P:                  ob_open();               break;
	case MOD_CTRL | KEY_I:                  tb_ins_include(_tb);     break;
	case MOD_CTRL | MOD_SHIFT | KEY_I:      tb_ins_include_lib(_tb); break;
	case MOD_CTRL | MOD_SHIFT | KEY_A:      tb_ins_comment(_tb);     break;
	case MOD_CTRL | KEY_A:                  tb_sel_all(_tb);         break;
	case MOD_CTRL | KEY_TAB:                bf_cycle();              break;
	case MOD_CTRL | KEY_F:                  mode_search();           break;
	case MOD_CTRL | MOD_SHIFT | KEY_F:      mode_search_in_dir();    break;
	case MOD_CTRL | KEY_H:                  mode_replace();          break;
	case MOD_CTRL | MOD_SHIFT | KEY_H:      mode_replace_in_dir();   break;
	case MOD_SHIFT | KEY_TAB:               tb_sel_unindent(_tb);    break;
	case MOD_CTRL | KEY_E:                  tb_align_defines(_tb);   break;
	case MOD_CTRL | KEY_T:                  cmd_open();              break;
	default:
		if(cp)
		{
			tb_char(_tb, cp);
		}
		break;
	}
}

static void event_scroll(i32 y)
{
	if(!_tb) { return; }
	tb_scroll(_tb, -3 * y);
}

static void event_dblclick(u32 x, u32 y)
{
	if(!_tb) { return; }
	tb_double_click(_tb, x, y);
}

static void event_tripleclick(u32 x, u32 y)
{
	if(!_tb) { return; }
	tb_triple_click(_tb, x, y);
}

static void event_mousedown(u32 x, u32 y)
{
	if(!_tb) { return; }
	tb_mouse_cursor(_tb, x, y);
}

static void event_shift_mousedown(u32 x, u32 y)
{
	if(!_tb) { return; }
	tb_mouse_sel(_tb, x, y);
}

static void event_mousemove(u32 x, u32 y)
{
	if(!_tb) { return; }
	tb_mouse_sel(_tb, x, y);
}

static void event_key(u32 key, u32 chr)
{
	switch(_mode)
	{
	case MODE_DEFAULT:
		default_key(key, chr);
		break;

	case MODE_OPEN:
		op_key(key, chr);
		break;

	case MODE_GOTO:
		gt_key(key, chr);
		break;

	case MODE_SAVE_AS:
		sv_key(key, chr);
		break;

	case MODE_OPENED:
		ob_key(key);
		break;

	case MODE_CONFIRM:
		cf_key(key);
		break;

	case MODE_SEARCH:
		sr_key(key, chr);
		break;
	}

	switch(key)
	{
	case MOD_CTRL | KEY_F3: test_run_all(); break;
	}
}

static void event_init(void)
{
	ed_init();
}

static void event_render(void)
{
	ed_render();
}

static u32 event_exit(void)
{
	if(bf_has_modified())
	{
		ob_open();
		return 0;
	}

	ed_destroy();
	return 1;
}
