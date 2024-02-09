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

static void *_mallocopy(const void *buf, size_t size)
{
	char *p = _malloc(size);
	memcpy(p, buf, size);
	return p;
}

static void vector_init(Vector *vector, u32 capacity)
{
	vector->Capacity = capacity;
	vector->Length = 0;
	vector->Data = _malloc(capacity);
}

static void vector_from(Vector *vector, const void *buf, u32 bytes)
{
	vector->Capacity = _next_pot(bytes);
	vector->Length = bytes;
	vector->Data = _mallocopy(buf, bytes);
}

static void vector_replace(
	Vector *vector, u32 index, u32 count, const void *elems, u32 new_count)
{
	u32 new_length;

	assert(index + count <= vector->Length);

	/* Calculate new number of elements */
	new_length = vector->Length - count + new_count;
	if(new_length > vector->Capacity)
	{
		/* Resize necessary */
		u32 new_capacity, prev_bytes, new_bytes, old_bytes, last_bytes;
		void *new_data;

		new_capacity = _next_pot(new_length);
		prev_bytes = index;
		new_bytes = new_count;
		old_bytes = count;
		last_bytes = (vector->Length - index - count);

		/* Calculate new capacity */
		vector->Capacity = new_capacity;

		/* Create new buffer */
		new_data = _malloc(new_capacity);

		/* Copy first part from previous buffer */
		memcpy(new_data, vector->Data, prev_bytes);

		/* Copy new range */
		memcpy((u8 *)new_data + prev_bytes, elems, new_bytes);

		/* Copy last part */
		memcpy((u8 *)new_data + prev_bytes + new_bytes,
			(u8 *)vector->Data + prev_bytes + old_bytes,
			last_bytes);

		/* Replace with new buffer */
		_free(vector->Data);
		vector->Data = new_data;
	}
	else
	{
		u32 prev_bytes, new_bytes, old_bytes, last_bytes;

		prev_bytes = index;
		new_bytes = new_count;
		old_bytes = count;
		last_bytes = (vector->Length - index - count);

		/* Shift last part */
		memmove((u8 *)vector->Data + prev_bytes + new_bytes,
			(u8 *)vector->Data + prev_bytes + old_bytes,
			last_bytes);

		/* Copy new range */
		memcpy((u8 *)vector->Data + prev_bytes, elems, new_bytes);
	}

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
