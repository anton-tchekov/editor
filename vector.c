typedef struct
{
	u32 capacity, len;
	void *data;
} vector;

static u32 _next_pot(u32 n)
{
	u32 power = 1;
	while(power < n)
	{
		power <<= 1;
	}

	return power;
}

static void vector_init(vector *v, u32 capacity)
{
	v->capacity = capacity;
	v->len = 0;
	v->data = _malloc(capacity);
}

static void vector_from(vector *v, void *buf, u32 bytes)
{
	v->capacity = bytes;
	v->len = bytes;
	v->data = _malloc(v->capacity);
	memcpy(v->data, buf, bytes);
}

static void vector_makespace(vector *v, u32 pos, u32 bytes)
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

static void vector_reserve(vector *v, u32 capacity)
{
	if(v->capacity < capacity)
	{
		v->capacity = _next_pot(capacity);
		v->data = _realloc(v->data, v->capacity);
	}
}

static void vector_replace(
	vector *v, u32 index, u32 count, void *elems, u32 new_count)
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

static void vector_destroy(vector *v)
{
	_free(v->data);
}

static void *vector_get(vector *v, u32 offset)
{
	assert(offset < v->len);
	return (u8 *)v->data + offset;
}

static void *vector_data(vector *v)
{
	return v->data;
}

static u32 vector_len(vector *v)
{
	return v->len;
}

static void vector_insert(vector *v, u32 offset, u32 bytes, void *elems)
{
	assert(offset <= v->len);
	vector_replace(v, offset, 0, elems, bytes);
}

static void vector_remove(vector *v, u32 offset, u32 bytes)
{
	assert(offset <= v->len);
	assert(bytes <= v->len);
	assert((offset + bytes) <= v->len);
	vector_replace(v, offset, bytes, NULL, 0);
}

static void vector_push(vector *v, u32 bytes, void *elem)
{
	vector_insert(v, v->len, bytes, elem);
}
