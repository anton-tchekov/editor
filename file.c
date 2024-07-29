#define FILE_CHUNK (16 * 1024)

enum
{
	FILE_READ_OK,
	FILE_READ_FAIL,
	FILE_READ_NOT_TEXT
};

static u32 is_text(u8 *s, size_t len)
{
	u32 c;
	u8 *end;
	for(end = s + len; s < end; ++s)
	{
		c = *s;
		if(!isprint(c) && c != '\n' && c != '\t')
		{
			return 0;
		}
	}

	return 1;
}

static u32 textfile_read(char *filename, char **out)
{
	vec v;
	u32 rb;
	FILE *fp;
	if(!(fp = fopen(filename, "r")))
	{
		return FILE_READ_FAIL;
	}

	vec_init(&v, FILE_CHUNK);
	for(;;)
	{
		rb = fread((u8 *)v.data + v.len, 1, FILE_CHUNK, fp);
		if(!is_text((u8 *)v.data + v.len, rb))
		{
			vec_destroy(&v);
			fclose(fp);
			return FILE_READ_NOT_TEXT;
		}

		v.len += rb;
		if(rb < FILE_CHUNK)
		{
			break;
		}

		vec_reserve(&v, v.len + FILE_CHUNK);
	}

	if(ferror(fp))
	{
		vec_destroy(&v);
		fclose(fp);
		return FILE_READ_FAIL;
	}

	{
		u8 nt[1];
		nt[0] = '\0';
		vec_push(&v, 1, nt);
	}

	*out = v.data;
	fclose(fp);
	return FILE_READ_OK;
}

static u32 file_write(char *filename, void *data, size_t len)
{
	FILE *fp;
	if(!(fp = fopen(filename, "w")))
	{
		return 1;
	}

	if(fwrite(data, 1, len, fp) != len)
	{
		return 1;
	}

	fclose(fp);
	return 0;
}

static u32 get_working_dir(char *buf)
{
	size_t len;
	getcwd(buf, PATH_MAX);
	len = strlen(buf);
	if(!len || (len > 0 && buf[len - 1] != '/'))
	{
		strcpy(buf + len, "/");
		++len;
	}

	return len;
}

static u32 file_exists(char *fname)
{
	return access(fname, F_OK) == 0;
}

static int dir_sort_callback(const void *a, const void *b)
{
	return strcmp(*(const char **)a, *(const char **)b);
}

static char **dir_sorted(const char *path, u32 *len)
{
	vec v;
	DIR *dir;
	struct dirent *dp;
	u32 i, count;
	char c[1];
	char *strs;
	char **ptrs;

	vec_init(&v, 1024);
	if(!(dir = opendir(path)))
	{
		return NULL;
	}

	count = 0;
	while((dp = readdir(dir)))
	{
		if(!strcmp(dp->d_name, "."))
		{
			continue;
		}

		vec_push(&v, strlen(dp->d_name), dp->d_name);
		if(dp->d_type == DT_DIR)
		{
			c[0] = '/';
			vec_push(&v, 1, c);
		}

		c[0] = '\0';
		vec_push(&v, 1, c);
		++count;
	}

	closedir(dir);
	vec_makespace(&v, 0, count * sizeof(char *));
	strs = (char *)v.data + count * sizeof(char *);
	ptrs = v.data;
	for(i = 0; i < count; ++i)
	{
		ptrs[i] = strs;
		strs += strlen(strs) + 1;
	}

	qsort(ptrs, count, sizeof(char *), dir_sort_callback);
	*len = count;
	return ptrs;
}
