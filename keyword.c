#define HASHMAP_SIZE       (ARRLEN(_keywords) * 7)
#define COLLISION_LIMIT   4

typedef struct
{
	u8 Color;
	const char *Name;
} Keyword;

static Keyword _keywords[] =
{
	{ COLOR_TABLE_TYPE, "const" },
	{ COLOR_TABLE_TYPE, "void" },
	{ COLOR_TABLE_TYPE, "bool" },
	{ COLOR_TABLE_TYPE, "char" },
	{ COLOR_TABLE_TYPE, "double" },
	{ COLOR_TABLE_TYPE, "int" },
	{ COLOR_TABLE_TYPE, "long" },

	{ COLOR_TABLE_TYPE, "f32" },
	{ COLOR_TABLE_TYPE, "f64" },

	{ COLOR_TABLE_TYPE, "i8" },
	{ COLOR_TABLE_TYPE, "i16" },
	{ COLOR_TABLE_TYPE, "i32" },
	{ COLOR_TABLE_TYPE, "i64" },

	{ COLOR_TABLE_TYPE, "u8" },
	{ COLOR_TABLE_TYPE, "u16" },
	{ COLOR_TABLE_TYPE, "u32" },
	{ COLOR_TABLE_TYPE, "u64" },

	{ COLOR_TABLE_TYPE, "int8_t" },
	{ COLOR_TABLE_TYPE, "int16_t" },
	{ COLOR_TABLE_TYPE, "int32_t" },
	{ COLOR_TABLE_TYPE, "int64_t" },

	{ COLOR_TABLE_TYPE, "uint8_t" },
	{ COLOR_TABLE_TYPE, "uint16_t" },
	{ COLOR_TABLE_TYPE, "uint32_t" },
	{ COLOR_TABLE_TYPE, "uint64_t" },

	{ COLOR_TABLE_TYPE, "size_t" },
	{ COLOR_TABLE_TYPE, "FILE" },

	{ COLOR_TABLE_KEYWORD, "break" },
	{ COLOR_TABLE_KEYWORD, "case" },
	{ COLOR_TABLE_KEYWORD, "continue" },
	{ COLOR_TABLE_KEYWORD, "default" },
	{ COLOR_TABLE_KEYWORD, "do" },
	{ COLOR_TABLE_KEYWORD, "if" },
	{ COLOR_TABLE_KEYWORD, "else" },
	{ COLOR_TABLE_KEYWORD, "enum" },
	{ COLOR_TABLE_KEYWORD, "extern" },
	{ COLOR_TABLE_KEYWORD, "false" },
	{ COLOR_TABLE_TYPE, "float" },
	{ COLOR_TABLE_KEYWORD, "for" },
	{ COLOR_TABLE_KEYWORD, "goto" },
	{ COLOR_TABLE_TYPE, "inline" },
	{ COLOR_TABLE_TYPE, "register" },
	{ COLOR_TABLE_KEYWORD, "return" },
	{ COLOR_TABLE_TYPE, "short" },
	{ COLOR_TABLE_TYPE, "signed" },
	{ COLOR_TABLE_TYPE, "sizeof" },
	{ COLOR_TABLE_TYPE, "static" },
	{ COLOR_TABLE_TYPE, "struct" },
	{ COLOR_TABLE_KEYWORD, "switch" },
	{ COLOR_TABLE_KEYWORD, "true" },
	{ COLOR_TABLE_TYPE, "typedef" },
	{ COLOR_TABLE_TYPE, "typeof" },
	{ COLOR_TABLE_TYPE, "union" },
	{ COLOR_TABLE_TYPE, "unsigned" },
	{ COLOR_TABLE_TYPE, "volatile" },
	{ COLOR_TABLE_KEYWORD, "while" },
};

static i32 _hashmap[HASHMAP_SIZE];

static u32 _keyword_hash(const char *word, u32 len)
{
	u32 i, hash, c;
	hash = 5381;
	for(i = 0; i < len && (c = word[i]); ++i)
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

static void keyword_init(void)
{
	u32 i, hash;

	for(i = 0; i < HASHMAP_SIZE; ++i)
	{
		_hashmap[i] = -1;
	}

	for(i = 0; i < ARRLEN(_keywords); ++i)
	{
#ifndef NDEBUG
		u32 steps = 0;
#endif
		hash = _keyword_hash(_keywords[i].Name, UINT32_MAX);
		while(_hashmap[hash % HASHMAP_SIZE] != -1)
		{
			++hash;
			assert(++steps < COLLISION_LIMIT);
		}

		_hashmap[hash % HASHMAP_SIZE] = i;
	}
}

static u32 keyword_detect(const char *str, u32 len)
{
	u32 i, hash;

	hash = _keyword_hash(str, len);
	for(i = 0; i < COLLISION_LIMIT; ++i)
	{
		i32 index = _hashmap[hash % HASHMAP_SIZE];
		if(index < 0)
		{
			break;
		}

		if(match_part(str, _keywords[index].Name, len))
		{
			return _keywords[index].Color;
		}

		++hash;
	}

	return 1;
}
