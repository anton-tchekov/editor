#define C_HASHMAP_SIZE       (ARRLEN(_c_keywords) * 7)
#define ASM_HASHMAP_SIZE     (ARRLEN(_asm_keywords) * 8)
#define COLLISION_LIMIT     4

typedef struct
{
	u8 color;
	char *name;
} keyword;

typedef struct
{
	size_t size;
	i32 *table;
	size_t num_keywords;
	keyword *keywords;
} hashmap;

static keyword _c_keywords[] =
{
	{ PT_TYPE, "asm" },

	{ PT_TYPE, "const" },
	{ PT_TYPE, "void" },
	{ PT_TYPE, "bool" },

	{ PT_TYPE, "char" },
	{ PT_TYPE, "double" },
	{ PT_TYPE, "int" },
	{ PT_TYPE, "long" },

	{ PT_TYPE, "f32" },
	{ PT_TYPE, "f64" },

	{ PT_TYPE, "i8" },
	{ PT_TYPE, "i16" },
	{ PT_TYPE, "i32" },
	{ PT_TYPE, "i64" },

	{ PT_TYPE, "u8" },
	{ PT_TYPE, "u16" },
	{ PT_TYPE, "u32" },
	{ PT_TYPE, "u64" },

	{ PT_TYPE, "int8_t" },
	{ PT_TYPE, "int16_t" },
	{ PT_TYPE, "int32_t" },
	{ PT_TYPE, "int64_t" },

	{ PT_TYPE, "uint8_t" },
	{ PT_TYPE, "uint16_t" },
	{ PT_TYPE, "uint32_t" },
	{ PT_TYPE, "uint64_t" },

	{ PT_TYPE, "size_t" },
	{ PT_TYPE, "FILE" },
	{ PT_TYPE, "NULL" },

	{ PT_KEYWORD, "break" },
	{ PT_KEYWORD, "case" },
	{ PT_KEYWORD, "continue" },
	{ PT_KEYWORD, "default" },
	{ PT_KEYWORD, "do" },
	{ PT_KEYWORD, "if" },
	{ PT_KEYWORD, "else" },
	{ PT_TYPE, "enum" },
	{ PT_KEYWORD, "extern" },
	{ PT_KEYWORD, "false" },
	{ PT_TYPE, "float" },
	{ PT_KEYWORD, "for" },
	{ PT_KEYWORD, "goto" },
	{ PT_TYPE, "inline" },
	{ PT_TYPE, "register" },
	{ PT_KEYWORD, "return" },
	{ PT_TYPE, "short" },
	{ PT_TYPE, "signed" },
	{ PT_TYPE, "sizeof" },
	{ PT_TYPE, "static" },
	{ PT_TYPE, "struct" },
	{ PT_KEYWORD, "switch" },
	{ PT_KEYWORD, "true" },
	{ PT_TYPE, "typedef" },
	{ PT_TYPE, "typeof" },
	{ PT_TYPE, "union" },
	{ PT_TYPE, "unsigned" },
	{ PT_TYPE, "volatile" },
	{ PT_KEYWORD, "while" },

	{ PT_KEYWORD, "private" },
	{ PT_KEYWORD, "public" },
	{ PT_KEYWORD, "friend" },
	{ PT_KEYWORD, "throw" },
	{ PT_KEYWORD, "try" },
	{ PT_KEYWORD, "catch" },
	{ PT_KEYWORD, "using" },
	{ PT_KEYWORD, "virtual" },
	{ PT_KEYWORD, "new" },
	{ PT_KEYWORD, "delete" },
	{ PT_KEYWORD, "namespace" },
	{ PT_KEYWORD, "protected" },
	{ PT_KEYWORD, "template" },
	{ PT_KEYWORD, "operator" },
	{ PT_KEYWORD, "class" },
	{ PT_KEYWORD, "this" },
	{ PT_KEYWORD, "function" },
	{ PT_KEYWORD, "fn" },
};

static i32 _c_table[C_HASHMAP_SIZE];
static hashmap _c_hashmap =
{
	C_HASHMAP_SIZE, _c_table,
	ARRLEN(_c_keywords), _c_keywords
};

static keyword _asm_keywords[] =
{
	{ PT_TYPE, "NOP" },

	{ PT_TYPE, "TAP" },
	{ PT_TYPE, "TPA" },
	{ PT_TYPE, "INX" },
	{ PT_TYPE, "DEX" },
	{ PT_TYPE, "CLV" },
	{ PT_TYPE, "SEV" },
	{ PT_TYPE, "CLC" },
	{ PT_TYPE, "SEC" },
	{ PT_TYPE, "CLI" },
	{ PT_TYPE, "SEI" },
	{ PT_TYPE, "SBA" },
	{ PT_TYPE, "CBA" },

	{ PT_TYPE, "TAB" },
	{ PT_TYPE, "TBA" },
	{ PT_TYPE, "DAA" },

	{ PT_TYPE, "ABA" },

	{ PT_TYPE, "BRA" },
	{ PT_TYPE, "BHI" },
	{ PT_TYPE, "BLS" },
	{ PT_TYPE, "BLO" },
	{ PT_TYPE, "BHS" },
	{ PT_TYPE, "BCC" },
	{ PT_TYPE, "BCS" },
	{ PT_TYPE, "BNE" },
	{ PT_TYPE, "BEQ" },
	{ PT_TYPE, "BVC" },
	{ PT_TYPE, "BVS" },
	{ PT_TYPE, "BPL" },
	{ PT_TYPE, "BMI" },
	{ PT_TYPE, "BGE" },
	{ PT_TYPE, "BLT" },
	{ PT_TYPE, "BGT" },
	{ PT_TYPE, "BLE" },
	{ PT_TYPE, "TSX" },
	{ PT_TYPE, "INS" },
	{ PT_TYPE, "PULA" },
	{ PT_TYPE, "PULB" },
	{ PT_TYPE, "DES" },
	{ PT_TYPE, "TXS" },
	{ PT_TYPE, "PSHA" },
	{ PT_TYPE, "PSHB" },
	{ PT_TYPE, "RTS" },
	{ PT_TYPE, "RTI" },
	{ PT_TYPE, "WAI" },
	{ PT_TYPE, "SWI" },

	{ PT_TYPE, "NEGA" },
	{ PT_TYPE, "COMA" },
	{ PT_TYPE, "LSRA" },
	{ PT_TYPE, "RORA" },
	{ PT_TYPE, "ASRA" },
	{ PT_TYPE, "ASLA" },
	{ PT_TYPE, "ROLA" },
	{ PT_TYPE, "DECA" },
	{ PT_TYPE, "INCA" },
	{ PT_TYPE, "TSTA" },
	{ PT_TYPE, "CLRA" },

	{ PT_TYPE, "NEGB" },
	{ PT_TYPE, "COMB" },
	{ PT_TYPE, "LSRB" },
	{ PT_TYPE, "RORB" },
	{ PT_TYPE, "ASRB" },
	{ PT_TYPE, "ASLB" },
	{ PT_TYPE, "ROLB" },
	{ PT_TYPE, "DECB" },
	{ PT_TYPE, "INCB" },
	{ PT_TYPE, "TSTB" },
	{ PT_TYPE, "CLRB" },

	{ PT_TYPE, "NEG" },
	{ PT_TYPE, "COM" },
	{ PT_TYPE, "LSR" },
	{ PT_TYPE, "ROR" },
	{ PT_TYPE, "ASR" },
	{ PT_TYPE, "ASL" },
	{ PT_TYPE, "ROL" },
	{ PT_TYPE, "DEC" },
	{ PT_TYPE, "INC" },
	{ PT_TYPE, "TST" },
	{ PT_TYPE, "CLR" },

	{ PT_TYPE, "JMP" },

	{ PT_TYPE, "SUBA" },
	{ PT_TYPE, "CMPA" },
	{ PT_TYPE, "SBCA" },
	{ PT_TYPE, "ANDA" },
	{ PT_TYPE, "BITA" },
	{ PT_TYPE, "LDAA" },
	{ PT_TYPE, "EORA" },
	{ PT_TYPE, "ADCA" },
	{ PT_TYPE, "ORAA" },
	{ PT_TYPE, "ADDA" },
	{ PT_TYPE, "CPXA" },
	{ PT_TYPE, "STAA" },

	{ PT_TYPE, "SUBB" },
	{ PT_TYPE, "CMPB" },
	{ PT_TYPE, "SBCB" },
	{ PT_TYPE, "ANDB" },
	{ PT_TYPE, "BITB" },
	{ PT_TYPE, "LDAB" },
	{ PT_TYPE, "EORB" },
	{ PT_TYPE, "ADCB" },
	{ PT_TYPE, "ORAB" },
	{ PT_TYPE, "ADDB" },
	{ PT_TYPE, "STAB" },

	{ PT_TYPE, "BSR" },
	{ PT_TYPE, "STS" },
	{ PT_TYPE, "LDS" },
	{ PT_TYPE, "LDX" },
	{ PT_TYPE, "STX" },
	{ PT_TYPE, "CPX" },
	{ PT_TYPE, "JSR" },

	{ PT_KEYWORD, "EQU" },
	{ PT_KEYWORD, "ORG" },
	{ PT_KEYWORD, "DC.W" },
	{ PT_KEYWORD, "DC.B" }
};

static i32 _asm_table[ASM_HASHMAP_SIZE];
static hashmap _asm_hashmap =
{
	ASM_HASHMAP_SIZE, _asm_table,
	ARRLEN(_asm_keywords), _asm_keywords
};

static u32 keyword_hash(char *word, u32 len)
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

static u32 keyword_detect(hashmap *hm, char *str, u32 len)
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
