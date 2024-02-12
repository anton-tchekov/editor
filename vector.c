typedef struct
{
	u32 Capacity;
	u32 Length;
	void *Data;
} Vector;

static u32 _next_pot(u32 n)
{
	u32 power = 1;
	while(power < n)
	{
		power <<= 1;
	}

	return power;
}

static void vector_init(Vector *vector, u32 capacity)
{
	vector->Capacity = capacity;
	vector->Length = 0;
	vector->Data = _malloc(capacity);
}

static void vector_from(Vector *vector, const void *buf, u32 bytes)
{
	vector->Capacity = bytes;
	vector->Length = bytes;
	vector->Data = _malloc(vector->Capacity);
	memcpy(vector->Data, buf, bytes);
}

static void vector_makespace(Vector *vector, u32 pos, u32 bytes)
{
	u32 new_length;
	u8 *offset;
	assert(pos <= vector->Length);
	new_length = vector->Length + bytes;
	if(new_length > vector->Capacity)
	{
		vector->Capacity = _next_pot(new_length);
		vector->Data = _realloc(vector->Data, vector->Capacity);
	}

	offset = (u8 *)vector->Data + pos;
	memmove(offset + bytes, offset, vector->Length - pos);
	vector->Length = new_length;
}

static void vector_reserve(Vector *vector, u32 capacity)
{
	if(vector->Capacity < capacity)
	{
		vector->Capacity = _next_pot(capacity);
		vector->Data = _realloc(vector->Data, vector->Capacity);
	}
}

static void vector_replace(
	Vector *vector, u32 index, u32 count, const void *elems, u32 new_count)
{
	u32 new_length;

	assert(index <= vector->Length);
	assert(count <= vector->Length);
	assert(index + count <= vector->Length);

	new_length = vector->Length - count + new_count;
	if(new_length > vector->Capacity)
	{
		vector->Capacity = _next_pot(new_length);
		vector->Data = _realloc(vector->Data, vector->Capacity);
	}

	memmove((u8 *)vector->Data + index + new_count,
		(u8 *)vector->Data + index + count,
		vector->Length - index - count);

	memcpy((u8 *)vector->Data + index, elems, new_count);
	vector->Length = new_length;
}

static void vector_clear(Vector *vector)
{
	vector->Length = 0;
}

static void vector_destroy(Vector *vector)
{
	_free(vector->Data);
}

static void *vector_get(Vector *vector, u32 offset)
{
	assert(offset < vector->Length);
	return (u8 *)vector->Data + offset;
}

static void *vector_data(Vector *vector)
{
	return vector->Data;
}

static u32 vector_len(Vector *vector)
{
	return vector->Length;
}

static void vector_insert(
	Vector *vector, u32 offset, u32 bytes, const void *elems)
{
	assert(offset <= vector->Length);
	vector_replace(vector, offset, 0, elems, bytes);
}

static void vector_remove(Vector *vector, u32 offset, u32 bytes)
{
	assert(offset <= vector->Length);
	assert(bytes <= vector->Length);
	assert((offset + bytes) <= vector->Length);
	vector_replace(vector, offset, bytes, NULL, 0);
}

static void vector_push(Vector *vector, u32 bytes, const void *elem)
{
	vector_insert(vector, vector->Length, bytes, elem);
}
