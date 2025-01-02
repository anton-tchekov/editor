#include <sys/stat.h>
#include <errno.h>

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

	vec v = vec_init(FILE_CHUNK);
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

static vec get_working_dir(void)
{
	vec v = vec_init(256);
	for(;;)
	{
		char *p = getcwd(v.data, v.capacity);
		if(p)
		{
			v.len = strlen(v.data);
			break;
		}

		if(errno == ERANGE)
		{
			vec_reserve(&v, 2 * v.capacity);
			continue;
		}

		fprintf(stderr, "Error reading current directory\n");
		exit(1);
	}

	if(!v.len || (v.len > 0 && vec_str(&v)[v.len - 1] != '/'))
	{
		vec_pushbyte(&v, '/');
	}

	return v;
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

// TODO: Use vec
static char **dir_sorted(vec *path, u32 *len)
{
	vec v = vec_init(1024);

	DIR *dir;
	if(!(dir = opendir(vec_cstr(path))))
	{
		return NULL;
	}

	vec curfile = vec_init(1024);

	u32 count = 0;
	struct dirent *dp;
	while((dp = readdir(dir)))
	{
		if(!strcmp(dp->d_name, "."))
		{
			continue;
		}

		u32 namelen = strlen(dp->d_name);
		vec_push(&v, namelen, dp->d_name);
		vec_strcpy(&curfile, path);
		vec_push(&curfile, namelen, dp->d_name);
		if(is_dir(vec_cstr(&curfile)))
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
