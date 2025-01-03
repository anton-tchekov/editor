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

		// TODO: Exponention increase
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
	return vec_strcmp((vec *)a, (vec *)b);
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

static u32 dir_sorted(vec *path, vec *list)
{
	DIR *dir = opendir(vec_cstr(path));
	if(!dir)
	{
		return 1;
	}

	*list = vec_init(128 * sizeof(vec));
	vec curfile = vec_copy(path);
	struct dirent *dp;
	while((dp = readdir(dir)))
	{
		if(!strcmp(dp->d_name, "."))
		{
			continue;
		}

		u32 namelen = strlen(dp->d_name);
		vec v = vec_init(namelen + 2);
		vec_push(&v, namelen, dp->d_name);

		curfile.len = path->len;
		vec_push(&curfile, namelen, dp->d_name);
		if(is_dir(vec_cstr(&curfile)))
		{
			vec_pushbyte(&v, '/');
		}

		vec_push(list, sizeof(vec), &v);
	}

	vec_destroy(&curfile);
	closedir(dir);
	qsort(list->data, vec_num_vecs(list), sizeof(vec), dir_sort_callback);
	return 0;
}
