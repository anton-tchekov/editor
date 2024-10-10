typedef struct
{
	u32 capacity, len;
	void *data;
} vec;

static u32 _next_pot(u32 n)
{
	u32 power;

	power = 1;
	while(power < n)
	{
		power <<= 1;
	}

	return power;
}

static void vec_init(vec *v, u32 capacity)
{
	v->capacity = capacity;
	v->len = 0;
	v->data = _malloc(capacity);
}

static void vec_init_full(vec *v, u32 capacity)
{
	v->capacity = capacity;
	v->len = capacity;
	v->data = _malloc(capacity);
}

static void vec_clear(vec *v)
{
	v->len = 0;
}

static void vec_from(vec *v, void *buf, u32 bytes)
{
	v->capacity = bytes;
	v->len = bytes;
	v->data = _malloc(v->capacity);
	memcpy(v->data, buf, bytes);
}

static void vec_makespace(vec *v, u32 pos, u32 bytes)
{
	u32 new_length;
	u8 *offset;

	assert(pos <= v->len);
	new_length = v->len + bytes;
	if(new_length > v->capacity)
	{
		v->capacity = _next_pot(new_length);
		v->data = _realloc(v->data, v->capacity);
	}

	offset = (u8 *)v->data + pos;
	memmove(offset + bytes, offset, v->len - pos);
	v->len = new_length;
}

static void vec_reserve(vec *v, u32 capacity)
{
	if(v->capacity < capacity)
	{
		v->capacity = _next_pot(capacity);
		v->data = _realloc(v->data, v->capacity);
	}
}

static void vec_replace(
	vec *v, u32 index, u32 count, void *elems, u32 new_count)
{
	u32 new_length;

	assert(index <= v->len);
	assert(count <= v->len);
	assert(index + count <= v->len);

	new_length = v->len - count + new_count;
	if(new_length > v->capacity)
	{
		v->capacity = _next_pot(new_length);
		v->data = _realloc(v->data, v->capacity);
	}

	memmove((u8 *)v->data + index + new_count,
		(u8 *)v->data + index + count,
		v->len - index - count);

	memcpy((u8 *)v->data + index, elems, new_count);
	v->len = new_length;
}

static void vec_destroy(vec *v)
{
	_free(v->data);
}

static void *vec_get(vec *v, u32 offset)
{
	assert(offset < v->len);
	return (u8 *)v->data + offset;
}

static void *vec_data(vec *v)
{
	return v->data;
}

static char *vec_str(vec *v)
{
	return v->data;
}

static void vec_str_clear(vec *v)
{
	char *s;

	vec_reserve(v, 1);
	s = v->data;
	s[0] = '\0';
	v->len = 1;
}

static u32 vec_len(vec *v)
{
	return v->len;
}

static void vec_insert(vec *v, u32 offset, u32 bytes, void *elems)
{
	assert(offset <= v->len);
	vec_replace(v, offset, 0, elems, bytes);
}

static void vec_remove(vec *v, u32 offset, u32 bytes)
{
	assert(offset <= v->len);
	assert(bytes <= v->len);
	assert((offset + bytes) <= v->len);
	vec_replace(v, offset, bytes, NULL, 0);
}

static void vec_push(vec *v, u32 bytes, void *elem)
{
	vec_insert(v, v->len, bytes, elem);
}

static void vec_pushbyte(vec *v, u32 c)
{
	char b[1] = { c };
	vec_push(v, 1, b);
}
