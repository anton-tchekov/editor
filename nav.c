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

	printf("Compare with: %s\n", fname);
	if(starts_with(fname, search_file))
	{
		printf("%s starts with %s\n", fname, search_file);
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
		u32 lnr;
		ed->Search[ed->SLen] = '\0';
		if((lnr = conv_lnr_str(ed->Search)) > 0)
		{
			if(lnr > ed->Lines.Length)
			{
				break;
			}

			ed->CursorY = lnr - 1;
			if(ed->CursorY > ed->PageH / 2)
			{
				ed->PageY = ed->CursorY - ed->PageH / 2;
			}

			ed->CursorX = 0;
			ed->CursorSaveX = 0;
			ed->Mode = EDITOR_MODE_DEFAULT;
			editor_render(ed);
		}
		else if(ed->SLen != 0)
		{
			editor_load(ed, ed->Search);
		}
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
		strcpy(buf, ed->Path);
		strcat(buf, ed->Search);
		path_dir(buf);
		first_compare = 1;
		search_file = path_file(ed->Search);
		if(dir_iter(buf, ed, foreach_dirent))
		{
			editor_msg(ed, EDITOR_ERROR, "Failed to open directory");
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
		ed->Mode = EDITOR_MODE_DEFAULT;
		editor_render(ed);
		break;

	default:
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
