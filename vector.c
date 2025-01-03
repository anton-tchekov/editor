typedef struct
{
	u32 capacity, len;
	void *data;
} vec;

static u32 _next_pot(u32 n)
{
	u32 power = 1;
	while(power < n)
	{
		power <<= 1;
	}

	return power;
}

static vec vec_init(u32 capacity)
{
	vec v;
	v.capacity = capacity;
	v.len = 0;
	v.data = _malloc(capacity);
	return v;
}

static void vec_clear(vec *v)
{
	v->len = 0;
}

static vec vec_from(void *buf, u32 bytes)
{
	vec v;
	v.capacity = bytes;
	v.len = bytes;
	v.data = _malloc(bytes);
	memcpy(v.data, buf, bytes);
	return v;
}

static void vec_makespace(vec *v, u32 pos, u32 bytes)
{
	assert(pos <= v->len);
	u32 new_length = v->len + bytes;
	if(new_length > v->capacity)
	{
		v->capacity = _next_pot(new_length);
		v->data = _realloc(v->data, v->capacity);
	}

	u8 *offset = (u8 *)v->data + pos;
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
	assert(index <= v->len);
	assert(count <= v->len);
	assert(index + count <= v->len);

	u32 new_length = v->len - count + new_count;
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
	vec_reserve(v, 1);
	char *s = v->data;
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

static void vec_pushn(vec *v, u32 byte, u32 count)
{
	vec_reserve(v, v->len + count);
	memset((u8 *)v->data + v->len, byte, count);
	v->len += count;
}

static void vec_pushbyte(vec *v, u32 c)
{
	char b[1] = { c };
	vec_push(v, 1, b);
}

static void vec_strcpy(vec *to, vec *from)
{
	vec_replace(to, 0, to->len, from->data, from->len);
}

static void vec_strcat(vec *to, vec *from)
{
	vec_push(to, from->len, from->data);
}

static char *vec_cstr(vec *v)
{
	vec_reserve(v, v->len + 1);
	((char *)v->data)[v->len] = '\0';
	return v->data;
}

static u32 vec_eq(vec *a, vec *b)
{
	if(a->len != b->len)
	{
		return 0;
	}

	return !memcmp(a->data, b->data, a->len);
}

static u32 vec_strcmp(vec *a, vec *b)
{
	return memcmp(a->data, b->data, umin(a->len, b->len));
}

static vec vec_copy(vec *v)
{
	vec to = vec_init(v->len);
	vec_strcpy(&to, v);
	return to;
}

static vec *vec_get_vec(vec *v, u32 i)
{
	return ((vec *)v->data) + i;
}

static u32 vec_num_vecs(vec *v)
{
	return v->len / sizeof(vec);
}
