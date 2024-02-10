/* TODO: Search bar max capacity */

static void ed_dir_load_callback(void *data, const char *fname, u32 is_dir)
{
	Editor *ed = data;
	if(ed->DirIdx < ED_DIR_BUF_SIZE)
	{
		++ed->DirEntries;
		for(; *fname; ++fname)
		{
			ed->DirBuf[ed->DirIdx++] = *fname;
		}

		if(is_dir)
		{
			ed->DirBuf[ed->DirIdx++] = '/';
		}

		ed->DirBuf[ed->DirIdx++] = '\0';
	}
	else
	{
		++ed->DirOverflow;
	}
}

static void ed_dir_load(Editor *ed)
{
	char buf[256]; /* TODO: POTENTIAL OVERFLOW HAZARD */

	ed->DirPos = 0;
	ed->DirOffset = 0;
	ed->DirEntries = 0;
	ed->DirOverflow = 0;
	ed->DirIdx = 0;

	ed->Search[ed->SLen] = '\0';
	strcpy(buf, ed->SBuf);
	path_dir(buf);

	dir_iter(buf, ed, ed_dir_load_callback);

	ed->DirIdx = 0;
}

static void ed_nav_open(Editor *ed)
{
	ed->Mode = EDITOR_MODE_GOTO;
	ed_dir_load(ed);
	ed_render(ed);
}

static void ed_nav_close(Editor *ed)
{
	ed->Mode = EDITOR_MODE_DEFAULT;
	ed_render(ed);
}

static void ed_goto(Editor *ed)
{
	ed->Search[0] = ':';
	ed->SCursor = 1;
	ed->SLen = 1;
	ed_nav_open(ed);
}

static void ed_open(Editor *ed)
{
	ed->SLen = 0;
	ed->SCursor = 0;
	ed_nav_open(ed);
}

static void ed_tab_cmpl_callback(void *data, const char *fname, u32 is_dir)
{
	Editor *ed = data;
	if(!strcmp(fname, ".") || !strcmp(fname, ".."))
	{
		return;
	}

	if(starts_with(fname, ed->SearchFile))
	{
		if(ed->FirstCompare)
		{
			ed->FirstCompare = 0;
			strcpy(ed->Same, fname);
			if(is_dir)
			{
				strcat(ed->Same, "/");
			}
		}
		else
		{
			const char *q = fname;
			char *p = ed->Same;
			while(*p == *q) { ++p; ++q; }
			*p = '\0';
		}
	}
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

static size_t revstrlen(const char *p)
{
	size_t cnt = 0;
	--p;
	do
	{
		--p;
		++cnt;
	}
	while(*p);
	return cnt;
}

static void ed_key_press_nav(Editor *ed, u32 key, u32 cp)
{
	switch(key)
	{
	case KEY_UP:
		if(ed->DirPos > 0)
		{
			--ed->DirPos;
			if(ed->DirPos < ed->DirOffset)
			{
				--ed->DirOffset;
				ed->DirIdx -= revstrlen(ed->DirBuf + ed->DirIdx);
			}
			ed_render(ed);
		}
		break;

	case KEY_DOWN:
		if(ed->DirPos < ed->DirEntries - 1)
		{
			++ed->DirPos;
			if(ed->DirPos >= ed->DirOffset + ED_DIR_PAGE)
			{
				++ed->DirOffset;
				ed->DirIdx += strlen(ed->DirBuf + ed->DirIdx) + 1;
			}
			ed_render(ed);
		}
		break;

	case KEY_LEFT:
		if(ed->SCursor > 0)
		{
			--ed->SCursor;
		}
		ed_render(ed);
		break;

	case KEY_RIGHT:
		if(ed->SCursor < ed->SLen)
		{
			++ed->SCursor;
		}
		ed_render(ed);
		break;

	case KEY_HOME:
		ed->SCursor = 0;
		ed_render(ed);
		break;

	case KEY_END:
		ed->SCursor = ed->SLen;
		ed_render(ed);
		break;

	case KEY_RETURN:
	{
		char *p;
		if(!ed->SLen)
		{
			ed_nav_close(ed);
			break;
		}

		ed->Search[ed->SLen] = '\0';
		p = memchr(ed->Search, ':', ed->SLen);
		if(p)
		{
			*p = '\0';
			if(p != ed->Search)
			{
				ed_load(ed, ed->Search);
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
			ed_load(ed, ed->Search);
		}

		ed_render(ed);
		break;
	}

	case KEY_BACKSPACE:
		if(ed->SCursor > 0)
		{
			char *p = ed->Search + ed->SCursor;
			memmove(p - 1, p, ed->SLen - ed->SCursor);
			--ed->SCursor;
			--ed->SLen;
			ed_render(ed);
		}
		break;

	case KEY_DELETE:
		if(ed->SCursor < ed->SLen)
		{
			char *p = ed->Search + ed->SCursor;
			memmove(p, p + 1, ed->SLen - ed->SCursor - 1);
			--ed->SLen;
			ed_render(ed);
		}
		break;

	case KEY_TAB:
	{
		char buf[256]; /* TODO: POTENTIAL OVERFLOW HAZARD */
		ed->Search[ed->SLen] = '\0';
		strcpy(buf, ed->SBuf);
		path_dir(buf);
		ed->FirstCompare = 1;
		ed->SearchFile = path_file(ed->Search);
		if(dir_iter(buf, ed, ed_tab_cmpl_callback))
		{
			ed_msg(ed, EDITOR_ERROR, "Failed to open directory");
			break;
		}

		if(!ed->FirstCompare)
		{
			strcpy(ed->SearchFile, ed->Same);
			ed->SCursor = ed->SLen = strlen(ed->Search);
			ed_render(ed);
		}
		break;
	}

	case MOD_CTRL | KEY_G:
	case KEY_ESCAPE:
		ed_nav_close(ed);
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
			ed_render(ed);
		}
		break;
	}
}
