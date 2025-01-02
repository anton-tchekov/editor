const char *_configfile = "editor.ini";

enum
{
	CONFIG_U8,
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
	{ "TabSize", CONFIG_U32, &_tabsize }
};

static void config_write_defaults(FILE *fp)
{

}

static void config_write(void)
{
	FILE *fp = fopen(_configfile, "w");
	if(!fp)
	{
		fprintf(stderr, "Failed to open config file %s for writing\n",
			_configfile);
		return;
	}

	config_write_defaults(fp);
	fclose(fp);
}

static void process_kv(char *key, char *value)
{
	printf("Key: %s - Value: %s\n", key, value);
}

static void config_read(FILE *fp)
{
	char buf[128];
	int line = 0;
	while(fgets(buf, sizeof(buf), fp))
	{
		++line;

		printf("Read: %s", buf);
		int c = buf[0];
		if(c == ';' || c == '\n' || c == '\0')
		{
			continue;
		}

		char *sep = strchr(buf, '=');
		if(sep == NULL)
		{
			fprintf(stderr, "Expected key-value pair on line %d\n",
				line);
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
