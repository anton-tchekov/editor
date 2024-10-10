/* CMD = Command Mode */
typedef struct COMMAND
{
	char *Name;
	void (*Fn)(void);
} Command;

typedef struct
{
	Command *Entries;
	size_t Len;
} CommandList;

static void cmd_set_language(void)
{

}

static void cmd_set_tabsize(void)
{

}

static void cmd_indent_vis(void)
{

}

static void cmd_space_vis(void)
{

}

static void cmd_ln_vis(void)
{

}

static void cmd_align_defines(void)
{

}

static void cmd_upper(void)
{

}

static void cmd_lower(void)
{

}

static void cmd_snake(void)
{

}

static void cmd_camel(void)
{

}

static void cmd_kebab(void)
{

}

static void cmd_pascal(void)
{

}


static Command _commands[] =
{
	{ "Set Language",              cmd_set_language },
	{ "Set Tab Size",              cmd_set_tabsize },
	{ "Indentation Visibility",    cmd_indent_vis },
	{ "Whitespace Visibility",     cmd_space_vis },
	{ "Line Number Visibility",    cmd_ln_vis },
	{ "Selection: Align #defines", cmd_align_defines },
	{ "Selection to UPPERCASE",    cmd_upper },
	{ "Selection to lowercase",    cmd_lower },
	{ "Selection to snake_case",   cmd_snake },
	{ "Selection to camelCase",    cmd_camel },
	{ "Selection to kebab-case",   cmd_kebab },
	{ "Selection to PascalCase",   cmd_pascal }
};

static CommandList _cmd_list_main = { _commands, ARRLEN(_commands) };
static CommandList *_cmd_list_cur;

static void cmd_open(void)
{
	_cmd_list_cur = _cmd_list_main;
}

static void cmd_entry_render(u32 i, Command *cmd)
{

}

static void cmd_render(void)
{
	for(u32 i = 0; i < _cmd_list_cur.Len; ++i)
	{
		cmd_entry_render(i, _cmd_list_cur.Entries + i);
	}
}
