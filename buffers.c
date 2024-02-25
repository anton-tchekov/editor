static void bf_init(void)
{
	vector_init(&buffers, 8 * sizeof(textbuf *));
}

static u32 bf_count(void)
{
	return vector_len(&buffers) / sizeof(textbuf *);
}

static textbuf *bf_get(u32 i)
{
	return *(textbuf **)vector_get(&buffers, i * sizeof(textbuf *));
}

static void bf_destroy(void)
{
	u32 i, len = bf_count();
	for(i = 0; i < len; ++i)
	{
		tb_destroy(bf_get(i));
	}

	vector_destroy(&buffers);
}

static u32 bf_has_modified(void)
{
	u32 i, len = bf_count();
	for(i = 0; i < len; ++i)
	{
		if(bf_get(i)->modified)
		{
			return 1;
		}
	}

	return 0;
}

static void bf_switch_id(u32 i)
{
	cur_buf = i;
	tb = bf_get(i);
}

static u32 bf_switch_name(const char *name)
{
	u32 i, len = bf_count();
	for(i = 0; i < len; ++i)
	{
		textbuf *cur = bf_get(i);
		if(!strcmp(cur->filename, name))
		{
			cur_buf = i;
			tb = cur;
			return 1;
		}
	}

	return 0;
}

static void bf_cycle(void)
{
	u32 cnt = bf_count();
	if(cnt == 1)
	{
		return;
	}

	++cur_buf;
	if(cur_buf >= cnt)
	{
		cur_buf = 0;
	}

	tb = bf_get(cur_buf);
}

static void bf_insert_cur(textbuf *t)
{
	vector_push(&buffers, sizeof(textbuf *), &t);
	cur_buf = bf_count() - 1;
	tb = t;
}

static void bf_discard_cur(void)
{
	u32 cnt;
	tb_destroy(bf_get(cur_buf));
	vector_remove(&buffers, cur_buf * sizeof(textbuf *), sizeof(textbuf *));
	cnt = bf_count();
	if(!cnt)
	{
		tb = NULL;
	}
	else
	{
		if(cur_buf >= cnt)
		{
			cur_buf = 0;
		}

		tb = bf_get(cur_buf);
	}
}
