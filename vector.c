typedef struct
{
	u32 ElementSize;
	u32 Length;
	u32 Capacity;
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

static void vector_init(Vector *vector, u32 element_size, u32 initial_capacity)
{
	vector->ElementSize = element_size;
	vector->Capacity = initial_capacity;
	vector->Length = 0;
	vector->Data = _malloc(element_size * initial_capacity);
}

static void *mallocopy(const void *buf, size_t size)
{
	char *p = _malloc(size);
	memcpy(p, buf, size);
	return p;
}

static void vector_from(Vector *vector, const char *buf, u32 count)
{
	vector->ElementSize = 1;
	vector->Capacity = _next_pot(count);
	vector->Length = count;
	vector->Data = mallocopy(buf, count);
}

static void vector_replace(
	Vector *vector, u32 index, u32 count, const void *elems, u32 new_count)
{
	u32 new_length, element_size;
	element_size = vector->ElementSize;

	assert(index + count <= vector->Length);

	/* Calculate new number of elements */
	new_length = vector->Length - count + new_count;
	if(new_length > vector->Capacity)
	{
		/* Resize necessary */
		u32 new_capacity, prev_bytes, new_bytes, old_bytes, last_bytes;
		void *new_data;

		new_capacity = _next_pot(new_length);
		prev_bytes = index * element_size;
		new_bytes = new_count * element_size;
		old_bytes = count * element_size;
		last_bytes = (vector->Length - index - count) * element_size;

		/* Calculate new capacity */
		vector->Capacity = new_capacity;

		/* Create new buffer */
		new_data = _malloc(new_capacity * element_size);

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

		prev_bytes = index * element_size;
		new_bytes = new_count * element_size;
		old_bytes = count * element_size;
		last_bytes = (vector->Length - index - count) * element_size;

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

static void *vector_get(Vector *vector, u32 index)
{
	return (void *)((u8 *)vector->Data + vector->ElementSize * index);
}

static void *vector_data(Vector *vector)
{
	return vector->Data;
}

static u32 vector_len(Vector *vector)
{
	return vector->Length;
}

static void vector_insert_range(
	Vector *vector, u32 index, u32 count, const void *elems)
{
	vector_replace(vector, index, 0, elems, count);
}

static void vector_insert(Vector *vector, u32 index, const void *elem)
{
	vector_insert_range(vector, index, 1, elem);
}

static void vector_remove_range(Vector *vector, u32 index, u32 count)
{
	vector_replace(vector, index, count, NULL, 0);
}

static void vector_remove(Vector *vector, u32 index)
{
	vector_remove_range(vector, index, 1);
}

static void vector_push(Vector *vector, void *elem)
{
	vector_insert(vector, vector->Length, elem);
}

static void vector_push_range(Vector *vector, u32 count, const void *elem)
{
	vector_insert_range(vector, vector->Length, count, elem);
}
