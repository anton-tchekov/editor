static vec _buffers;
static u32 _cur_buf;
static u32 _untitled_cnt = 1;
static textbuf *_tb;

static void bf_init(void)
{
	_buffers = vec_init(8 * sizeof(textbuf *));
}

static u32 bf_count(void)
{
	return vec_len(&_buffers) / sizeof(textbuf *);
}

static textbuf *bf_get(u32 i)
{
	return *(textbuf **)vec_get(&_buffers, i * sizeof(textbuf *));
}

static void bf_destroy(void)
{
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		tb_destroy(bf_get(i));
	}

	vec_destroy(&_buffers);
}

static u32 bf_has_modified(void)
{
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		if(bf_get(i)->modified)
		{
			return 1;
		}
	}

	return 0;
}

static u32 bf_num_unsaved(void)
{
	u32 cnt = 0;
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		if(bf_get(i)->modified)
		{
			++cnt;
		}
	}

	return cnt;
}

static void bf_switch_id(u32 i)
{
	if(!_tb) { return; }
	_cur_buf = i;
	_tb = bf_get(i);
}

static u32 bf_switch_name(vec *name)
{
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		textbuf *cur = bf_get(i);
		if(vec_eq(&cur->filename, name))
		{
			_cur_buf = i;
			_tb = cur;
			return 1;
		}
	}

	return 0;
}

static void bf_cycle(void)
{
	u32 cnt = bf_count();
	if(cnt == 1) { return; }
	_cur_buf = inc_wrap(_cur_buf, cnt);
	_tb = bf_get(_cur_buf);
}

static void bf_insert_cur(textbuf *t)
{
	vec_push(&_buffers, sizeof(textbuf *), &t);
	_cur_buf = bf_count() - 1;
	_tb = t;
}

static void bf_discard(u32 i)
{
	tb_destroy(bf_get(i));
	vec_remove(&_buffers, i * sizeof(textbuf *), sizeof(textbuf *));
}

static void bf_discard_cur(void)
{
	bf_discard(_cur_buf);
	u32 cnt = bf_count();
	if(!cnt)
	{
		_tb = NULL;
	}
	else
	{
		if(_cur_buf >= cnt)
		{
			_cur_buf = 0;
		}

		_tb = bf_get(_cur_buf);
	}
}

static u32 bf_opened_and_modified(vec *name)
{
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		textbuf *cur = bf_get(i);
		if(vec_eq(&cur->filename, name))
		{
			return cur->modified;
		}
	}

	return 0;
}

static void bf_close_other(vec *name, u32 id)
{
	u32 len = bf_count();
	for(u32 i = 0; i < len; ++i)
	{
		if(vec_eq(&bf_get(i)->filename, name) && i != id)
		{
			bf_discard(i);
			return;
		}
	}
}
