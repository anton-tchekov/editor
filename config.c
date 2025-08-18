static char *_configfile = "editor.ini";

enum
{
	CONFIG_U8,
	CONFIG_BOOL,
	CONFIG_U32
};

typedef struct
{
	char *Name;
	int Type;
	void *Addr;
} ConfigItem;

static ConfigItem _config_items[] =
{
	{ "TabSize", CONFIG_U8, &_tabsize },
	{ "IndentSpaces", CONFIG_BOOL, &_usespaces },
	{ "ShowLinenumbers", CONFIG_BOOL, &_show_linenr },
	{ "ShowWhitespace", CONFIG_BOOL, &_show_whitespace },
	{ "LongLineMarker", CONFIG_U32, &_col_line },
	{ "LineSpacing", CONFIG_U32, &_line_spacing },
	{ "FontSize", CONFIG_U32, &_font_size }
};

static void get_value(ConfigItem *item, char *value, size_t len)
{
	switch(item->Type)
	{
	case CONFIG_U8:
		snprintf(value, len, "%d", *(u8 *)(item->Addr));
		break;

	case CONFIG_BOOL:
		snprintf(value, len, "%s", boolstr(*(u8 *)(item->Addr)));
		break;

	case CONFIG_U32:
		snprintf(value, len, "%d", *(u32 *)item->Addr);
		break;
	}
}

static void config_write(void)
{
	char line[64];
	char buf[32];
	vec v = vec_init(512);
	for(size_t i = 0; i < ARRLEN(_config_items); ++i)
	{
		ConfigItem *item = _config_items + i;
		get_value(item, buf, sizeof(buf));
		u32 len = snprintf(line, sizeof(line), "%s=%s\n", item->Name, buf);
		vec_push(&v, len, line);
	}

	file_write(_configfile, vec_str(&v), vec_len(&v));
	vec_destroy(&v);
}

static u32 isnumber(char *s)
{
	for(u32 c; (c = *s); ++s)
	{
		if(!isdigit(c))
		{
			return 0;
		}
	}

	return 1;
}

static u32 parse_value(char *value, u32 *val)
{
	if(!strcmp(value, "true"))
	{
		*val = 1;
		return 0;
	}

	if(!strcmp(value, "false"))
	{
		*val = 0;
		return 0;
	}

	if(isnumber(value))
	{
		*val = atoi(value);
		return 0;
	}

	return 1;
}

static void apply_value(ConfigItem *item, char *value)
{
	u32 n = 0;
	if(parse_value(value, &n))
	{
		fprintf(stderr, "Invalid value %s\n", value);
		return;
	}

	switch(item->Type)
	{
	case CONFIG_U8:
		*(u8 *)(item->Addr) = n;
		break;

	case CONFIG_BOOL:
		*(u8 *)(item->Addr) = n;
		break;

	case CONFIG_U32:
		*(u32 *)item->Addr = n;
		break;
	}
}

static void process_kv(char *key, char *value)
{
	printf("Key: %s - Value: %s\n", key, value);
	for(size_t i = 0; i < ARRLEN(_config_items); ++i)
	{
		ConfigItem *item = _config_items + i;
		if(!strcmp(key, item->Name))
		{
			apply_value(item, value);
			return;
		}
	}

	fprintf(stderr, "Invalid key: %s\n", key);
}

static void config_read(FILE *fp)
{
	char buf[128];
	int line = 0;
	while(fgets(buf, sizeof(buf), fp))
	{
		++line;

		char *e = strchr(buf, '\n');
		if(e)
		{
			*e = '\0';
		}

		// printf("Read: %s\n", buf);
		int c = buf[0];
		if(c == ';' || c == '\n' || c == '\0')
		{
			continue;
		}

		char *sep = strchr(buf, '=');
		if(sep == NULL)
		{
			fprintf(stderr, "Expected key-value pair on line %d\n", line);
			return;
		}

		*sep = '\0';
		process_kv(buf, sep + 1);
	}

	if(ferror(fp))
	{
		fprintf(stderr, "I/O Error while reading config file %s\n",
			_configfile);
		return;
	}
}

static void config_load(void)
{
	FILE *fp = fopen(_configfile, "r");
	if(!fp)
	{
		config_write();
		return;
	}

	config_read(fp);
	fclose(fp);
}
