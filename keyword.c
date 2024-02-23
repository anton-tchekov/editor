#define C_HASHMAP_SIZE       (ARRLEN(c_keywords) * 7)
#define ASM_HASHMAP_SIZE     (ARRLEN(asm_keywords) * 8)
#define COLLISION_LIMIT     4

typedef struct
{
	u8 color;
	const char *name;
} keyword;

typedef struct
{
	size_t size;
	i32 *table;
	size_t num_keywords;
	keyword *keywords;
} hashmap;

static keyword c_keywords[] =
{
	{ COLOR_TABLE_TYPE, "asm" },

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
	{ COLOR_TABLE_TYPE, "NULL" },

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

	{ COLOR_TABLE_KEYWORD, "private" },
	{ COLOR_TABLE_KEYWORD, "public" },
	{ COLOR_TABLE_KEYWORD, "friend" },
	{ COLOR_TABLE_KEYWORD, "throw" },
	{ COLOR_TABLE_KEYWORD, "try" },
	{ COLOR_TABLE_KEYWORD, "catch" },
	{ COLOR_TABLE_KEYWORD, "using" },
	{ COLOR_TABLE_KEYWORD, "virtual" },
	{ COLOR_TABLE_KEYWORD, "new" },
	{ COLOR_TABLE_KEYWORD, "delete" },
	{ COLOR_TABLE_KEYWORD, "namespace" },
	{ COLOR_TABLE_KEYWORD, "protected" },
	{ COLOR_TABLE_KEYWORD, "template" },
	{ COLOR_TABLE_KEYWORD, "operator" },
	{ COLOR_TABLE_KEYWORD, "class" },
	{ COLOR_TABLE_KEYWORD, "this" },
	{ COLOR_TABLE_KEYWORD, "function" },
	{ COLOR_TABLE_KEYWORD, "fn" },
};

static i32 c_table[C_HASHMAP_SIZE];
static hashmap c_hashmap =
{
	C_HASHMAP_SIZE, c_table,
	ARRLEN(c_keywords), c_keywords
};

static keyword asm_keywords[] =
{
	{ COLOR_TABLE_TYPE, "NOP" },

	{ COLOR_TABLE_TYPE, "TAP" },
	{ COLOR_TABLE_TYPE, "TPA" },
	{ COLOR_TABLE_TYPE, "INX" },
	{ COLOR_TABLE_TYPE, "DEX" },
	{ COLOR_TABLE_TYPE, "CLV" },
	{ COLOR_TABLE_TYPE, "SEV" },
	{ COLOR_TABLE_TYPE, "CLC" },
	{ COLOR_TABLE_TYPE, "SEC" },
	{ COLOR_TABLE_TYPE, "CLI" },
	{ COLOR_TABLE_TYPE, "SEI" },
	{ COLOR_TABLE_TYPE, "SBA" },
	{ COLOR_TABLE_TYPE, "CBA" },

	{ COLOR_TABLE_TYPE, "TAB" },
	{ COLOR_TABLE_TYPE, "TBA" },
	{ COLOR_TABLE_TYPE, "DAA" },

	{ COLOR_TABLE_TYPE, "ABA" },

	{ COLOR_TABLE_TYPE, "BRA" },
	{ COLOR_TABLE_TYPE, "BHI" },
	{ COLOR_TABLE_TYPE, "BLS" },
	{ COLOR_TABLE_TYPE, "BCC" },
	{ COLOR_TABLE_TYPE, "BCS" },
	{ COLOR_TABLE_TYPE, "BNE" },
	{ COLOR_TABLE_TYPE, "BEQ" },
	{ COLOR_TABLE_TYPE, "BVC" },
	{ COLOR_TABLE_TYPE, "BVS" },
	{ COLOR_TABLE_TYPE, "BPL" },
	{ COLOR_TABLE_TYPE, "BMI" },
	{ COLOR_TABLE_TYPE, "BGE" },
	{ COLOR_TABLE_TYPE, "BLT" },
	{ COLOR_TABLE_TYPE, "BGT" },
	{ COLOR_TABLE_TYPE, "BLE" },
	{ COLOR_TABLE_TYPE, "TSX" },
	{ COLOR_TABLE_TYPE, "INS" },
	{ COLOR_TABLE_TYPE, "PULA" },
	{ COLOR_TABLE_TYPE, "PULB" },
	{ COLOR_TABLE_TYPE, "DES" },
	{ COLOR_TABLE_TYPE, "TXS" },
	{ COLOR_TABLE_TYPE, "PSHA" },
	{ COLOR_TABLE_TYPE, "PSHB" },
	{ COLOR_TABLE_TYPE, "RTS" },
	{ COLOR_TABLE_TYPE, "RTI" },
	{ COLOR_TABLE_TYPE, "WAI" },
	{ COLOR_TABLE_TYPE, "SWI" },

	{ COLOR_TABLE_TYPE, "NEGA" },
	{ COLOR_TABLE_TYPE, "COMA" },
	{ COLOR_TABLE_TYPE, "LSRA" },
	{ COLOR_TABLE_TYPE, "RORA" },
	{ COLOR_TABLE_TYPE, "ASRA" },
	{ COLOR_TABLE_TYPE, "ASLA" },
	{ COLOR_TABLE_TYPE, "ROLA" },
	{ COLOR_TABLE_TYPE, "DECA" },
	{ COLOR_TABLE_TYPE, "INCA" },
	{ COLOR_TABLE_TYPE, "TSTA" },
	{ COLOR_TABLE_TYPE, "CLRA" },

	{ COLOR_TABLE_TYPE, "NEGB" },
	{ COLOR_TABLE_TYPE, "COMB" },
	{ COLOR_TABLE_TYPE, "LSRB" },
	{ COLOR_TABLE_TYPE, "RORB" },
	{ COLOR_TABLE_TYPE, "ASRB" },
	{ COLOR_TABLE_TYPE, "ASLB" },
	{ COLOR_TABLE_TYPE, "ROLB" },
	{ COLOR_TABLE_TYPE, "DECB" },
	{ COLOR_TABLE_TYPE, "INCB" },
	{ COLOR_TABLE_TYPE, "TSTB" },
	{ COLOR_TABLE_TYPE, "CLRB" },

	{ COLOR_TABLE_TYPE, "NEG" },
	{ COLOR_TABLE_TYPE, "COM" },
	{ COLOR_TABLE_TYPE, "LSR" },
	{ COLOR_TABLE_TYPE, "ROR" },
	{ COLOR_TABLE_TYPE, "ASR" },
	{ COLOR_TABLE_TYPE, "ASL" },
	{ COLOR_TABLE_TYPE, "ROL" },
	{ COLOR_TABLE_TYPE, "DEC" },
	{ COLOR_TABLE_TYPE, "INC" },
	{ COLOR_TABLE_TYPE, "TST" },
	{ COLOR_TABLE_TYPE, "CLR" },

	{ COLOR_TABLE_TYPE, "JMP" },

	{ COLOR_TABLE_TYPE, "SUBA" },
	{ COLOR_TABLE_TYPE, "CMPA" },
	{ COLOR_TABLE_TYPE, "SBCA" },
	{ COLOR_TABLE_TYPE, "ANDA" },
	{ COLOR_TABLE_TYPE, "BITA" },
	{ COLOR_TABLE_TYPE, "LDAA" },
	{ COLOR_TABLE_TYPE, "EORA" },
	{ COLOR_TABLE_TYPE, "ADCA" },
	{ COLOR_TABLE_TYPE, "ORA" },
	{ COLOR_TABLE_TYPE, "ADDA" },
	{ COLOR_TABLE_TYPE, "CPXA" },
	{ COLOR_TABLE_TYPE, "STAA" },

	{ COLOR_TABLE_TYPE, "SUBB" },
	{ COLOR_TABLE_TYPE, "CMPB" },
	{ COLOR_TABLE_TYPE, "SBCB" },
	{ COLOR_TABLE_TYPE, "ANDB" },
	{ COLOR_TABLE_TYPE, "BITB" },
	{ COLOR_TABLE_TYPE, "LDAB" },
	{ COLOR_TABLE_TYPE, "EORB" },
	{ COLOR_TABLE_TYPE, "ADCB" },
	{ COLOR_TABLE_TYPE, "ORB" },
	{ COLOR_TABLE_TYPE, "ADDB" },
	{ COLOR_TABLE_TYPE, "STAB" },

	{ COLOR_TABLE_TYPE, "BSR" },
	{ COLOR_TABLE_TYPE, "STS" },
	{ COLOR_TABLE_TYPE, "LDS" },
	{ COLOR_TABLE_TYPE, "LDX" },
	{ COLOR_TABLE_TYPE, "STX" },
	{ COLOR_TABLE_TYPE, "CPX" },
	{ COLOR_TABLE_TYPE, "JSR" },

	{ COLOR_TABLE_KEYWORD, "EQU" },
	{ COLOR_TABLE_KEYWORD, "ORG" },
	{ COLOR_TABLE_KEYWORD, "DC.W" },
	{ COLOR_TABLE_KEYWORD, "DC.B" }
};

static i32 asm_table[ASM_HASHMAP_SIZE];
static hashmap asm_hashmap =
{
	ASM_HASHMAP_SIZE, asm_table,
	ARRLEN(asm_keywords), asm_keywords
};

static u32 keyword_hash(const char *word, u32 len)
{
	u32 i, hash, c;
	hash = 5381;
	for(i = 0; i < len && (c = word[i]); ++i)
	{
		hash = ((hash << 5) + hash) + c;
	}

	return hash;
}

static void keyword_init(hashmap *hm)
{
	u32 i, hash;
	for(i = 0; i < hm->size; ++i)
	{
		hm->table[i] = -1;
	}

	for(i = 0; i < hm->num_keywords; ++i)
	{
#ifndef NDEBUG
		u32 steps = 0;
#endif
		hash = keyword_hash(hm->keywords[i].name, UINT32_MAX);
		while(hm->table[hash % hm->size] != -1)
		{
			++hash;
			assert(++steps < COLLISION_LIMIT);
		}

		hm->table[hash % hm->size] = i;
	}
}

static u32 keyword_detect(hashmap *hm, const char *str, u32 len)
{
	u32 i, hash;

	hash = keyword_hash(str, len);
	for(i = 0; i < COLLISION_LIMIT; ++i)
	{
		i32 index = hm->table[hash % hm->size];
		if(index < 0)
		{
			break;
		}

		if(match_part(str, hm->keywords[index].name, len))
		{
			return hm->keywords[index].color;
		}

		++hash;
	}

	return 1;
}
