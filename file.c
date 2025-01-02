#include <sys/stat.h>

#define FILE_CHUNK (16 * 1024)

enum
{
	FILE_READ_OK,
	FILE_READ_FAIL,
	FILE_READ_NOT_TEXT
};

static u32 file_read(char *filename, char **out, u32 *len)
{
	FILE *fp;
	if(!(fp = fopen(filename, "r")))
	{
		return FILE_READ_FAIL;
	}

	vec v;
	vec_init(&v, FILE_CHUNK);
	for(;;)
	{
		u32 rb = fread((u8 *)v.data + v.len, 1, FILE_CHUNK, fp);
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

	*len = v.len;
	vec_pushbyte(&v, '\0');
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
	getcwd(buf, 4096);
	size_t len = strlen(buf);
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

static int is_dir(char *path)
{
	struct stat statbuf;
	if(stat(path, &statbuf) != 0)
	{
		return 0;
	}

	return S_ISDIR(statbuf.st_mode);
}

static char **dir_sorted(char *path, u32 *len)
{
	vec v;
	vec_init(&v, 1024);

	DIR *dir;
	if(!(dir = opendir(path)))
	{
		return NULL;
	}

	vec curfile;
	vec_init(&curfile, 1024);

	u32 count = 0;
	struct dirent *dp;
	while((dp = readdir(dir)))
	{
		if(!strcmp(dp->d_name, "."))
		{
			continue;
		}

		vec_push(&v, strlen(dp->d_name), dp->d_name);
		
		vec_clear(&curfile);
		vec_push(&curfile, strlen(path), path);
		vec_push(&curfile, strlen(dp->d_name), dp->d_name);
		vec_pushbyte(&curfile, '\0');

		if(is_dir(vec_str(&curfile)))
		{
			vec_pushbyte(&v, '/');
		}

		vec_pushbyte(&v, '\0');
		++count;
	}

	vec_destroy(&curfile);

	closedir(dir);
	vec_makespace(&v, 0, count * sizeof(char *));
	char *strs = (char *)v.data + count * sizeof(char *);
	char **ptrs = v.data;
	for(u32 i = 0; i < count; ++i)
	{
		ptrs[i] = strs;
		strs += strlen(strs) + 1;
	}

	qsort(ptrs, count, sizeof(char *), dir_sort_callback);
	*len = count;
	return ptrs;
}
