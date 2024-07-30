static size_t _alloc_cnt, _free_cnt;

static void _alloc_fail(size_t size)
{
	fprintf(stderr, "Memory allocation failure (%zu bytes)\n", size);
	exit(1);
}

static void *_malloc(size_t size)
{
	void *p = malloc(size);
	if(!p) { _alloc_fail(size); }
	++_alloc_cnt;
	return p;
}

static void *_realloc(void *p, size_t size)
{
	if(!p)
	{
		++_alloc_cnt;
	}

	p = realloc(p, size);
	if(!p) { _alloc_fail(size); }
	return p;
}

static void _free(void *p)
{
	if(p)
	{
		free(p);
		++_free_cnt;
	}
}

static void _alloc_report(void)
{
	printf("%zu allocs, %zu frees\n", _alloc_cnt, _free_cnt);
}
