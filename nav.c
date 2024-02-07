/* FIXME: Search bar max capacity */

static char *search_file;
static u8 first_compare;
static char same[64];

static void foreach_dirent(void *data, const char *fname, u32 is_dir)
{
	Editor *ed = data;

	if(!strcmp(fname, ".") || !strcmp(fname, ".."))
	{
		return;
	}

	if(starts_with(fname, search_file))
	{
		if(first_compare)
		{
			first_compare = 0;
			strcpy(same, fname);
			if(is_dir)
			{
				strcat(same, "/");
			}
		}
		else
		{
			const char *q = fname;
			char *p = same;
			while(*p == *q) { ++p; ++q; }
			*p = '\0';
		}
	}
}

static void close_nav(Editor *ed)
{
	ed->Mode = EDITOR_MODE_DEFAULT;
	editor_render(ed);
}

static u32 ed_set_lnr(Editor *ed, const char *s)
{
	u32 lnr = conv_lnr_str(s);
	if(!lnr)
	{
		return 1;
	}

	ed_gotoxy(ed, 0, lnr - 1);
	return 0;
}

static void editor_key_press_nav(Editor *ed, u32 key, u32 cp)
{
	switch(key)
	{
	case KEY_LEFT:
		if(ed->SCursor > 0)
		{
			--ed->SCursor;
		}
		editor_render(ed);
		break;

	case KEY_RIGHT:
		if(ed->SCursor < ed->SLen)
		{
			++ed->SCursor;
		}
		editor_render(ed);
		break;

	case KEY_HOME:
		ed->SCursor = 0;
		editor_render(ed);
		break;

	case KEY_END:
		ed->SCursor = ed->SLen;
		editor_render(ed);
		break;

	case KEY_RETURN:
	{
		char *p;
		if(!ed->SLen)
		{
			close_nav(ed);
			break;
		}

		ed->Search[ed->SLen] = '\0';
		p = memchr(ed->Search, ':', ed->SLen);
		if(p)
		{
			*p = '\0';
			if(p != ed->Search)
			{
				editor_load(ed, ed->Search);
			}

			++p;
			if(ed_set_lnr(ed, p))
			{
				ed_goto_def(ed, p);
			}

			ed->Mode = EDITOR_MODE_DEFAULT;
		}
		else
		{
			editor_load(ed, ed->Search);
		}

		editor_render(ed);
		break;
	}

	case KEY_BACKSPACE:
		if(ed->SCursor > 0)
		{
			char *p = ed->Search + ed->SCursor;
			memmove(p - 1, p, ed->SLen - ed->SCursor);
			--ed->SCursor;
			--ed->SLen;
			editor_render(ed);
		}
		break;

	case KEY_DELETE:
		if(ed->SCursor < ed->SLen)
		{
			char *p = ed->Search + ed->SCursor;
			memmove(p, p + 1, ed->SLen - ed->SCursor - 1);
			--ed->SLen;
			editor_render(ed);
		}
		break;

	case KEY_TAB:
	{
		char buf[256]; /* FIXME: POTENTIAL OVERFLOW HAZARD */
		ed->Search[ed->SLen] = '\0';
		strcpy(buf, ed->SBuf);
		path_dir(buf);
		first_compare = 1;
		search_file = path_file(ed->Search);
		if(dir_iter(buf, ed, foreach_dirent))
		{
			editor_msg(ed, EDITOR_ERROR, "Failed to open directory");
			break;
		}

		if(!first_compare)
		{
			strcpy(search_file, same);
			ed->SCursor = ed->SLen = strlen(ed->Search);
			editor_render(ed);
		}
		break;
	}

	case MOD_CTRL | KEY_G:
	case KEY_ESCAPE:
		close_nav(ed);
		break;

	default:
		if(ed->SLen >= MAX_SEARCH_LEN - 1)
		{
			break;
		}

		if(isprint(cp))
		{
			char *p = ed->Search + ed->SCursor;
			memmove(p + 1, p, ed->SLen - ed->SCursor);
			ed->Search[ed->SCursor++] = cp;
			++ed->SLen;
			editor_render(ed);
		}
		break;
	}
}
