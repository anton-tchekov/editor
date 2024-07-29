#define COLLISION_LIMIT     1

typedef struct
{
	u32 color;
	char *name;
} keyword;

typedef struct
{
	size_t size;
	i32 *table;
	size_t num_keywords;
	keyword *keywords;
} hashmap;

static keyword _kw_c_keywords[] =
{
	{ COLOR_TYPE, "asm" },

	{ COLOR_TYPE, "const" },
	{ COLOR_TYPE, "void" },
	{ COLOR_TYPE, "bool" },

	{ COLOR_TYPE, "char" },
	{ COLOR_TYPE, "double" },
	{ COLOR_TYPE, "int" },
	{ COLOR_TYPE, "long" },

	{ COLOR_TYPE, "f32" },
	{ COLOR_TYPE, "f64" },

	{ COLOR_TYPE, "i8" },
	{ COLOR_TYPE, "i16" },
	{ COLOR_TYPE, "i32" },
	{ COLOR_TYPE, "i64" },

	{ COLOR_TYPE, "u8" },
	{ COLOR_TYPE, "u16" },
	{ COLOR_TYPE, "u32" },
	{ COLOR_TYPE, "u64" },

	{ COLOR_TYPE, "int8_t" },
	{ COLOR_TYPE, "int16_t" },
	{ COLOR_TYPE, "int32_t" },
	{ COLOR_TYPE, "int64_t" },

	{ COLOR_TYPE, "uint8_t" },
	{ COLOR_TYPE, "uint16_t" },
	{ COLOR_TYPE, "uint32_t" },
	{ COLOR_TYPE, "uint64_t" },

	{ COLOR_TYPE, "size_t" },
	{ COLOR_TYPE, "FILE" },
	{ COLOR_TYPE, "NULL" },

	{ COLOR_KEYWORD, "break" },
	{ COLOR_KEYWORD, "case" },
	{ COLOR_KEYWORD, "continue" },
	{ COLOR_KEYWORD, "default" },
	{ COLOR_KEYWORD, "do" },
	{ COLOR_KEYWORD, "if" },
	{ COLOR_KEYWORD, "else" },
	{ COLOR_TYPE, "enum" },
	{ COLOR_KEYWORD, "extern" },
	{ COLOR_KEYWORD, "false" },
	{ COLOR_TYPE, "float" },
	{ COLOR_KEYWORD, "for" },
	{ COLOR_KEYWORD, "goto" },
	{ COLOR_TYPE, "inline" },
	{ COLOR_TYPE, "register" },
	{ COLOR_KEYWORD, "return" },
	{ COLOR_TYPE, "short" },
	{ COLOR_TYPE, "signed" },
	{ COLOR_TYPE, "sizeof" },
	{ COLOR_TYPE, "static" },
	{ COLOR_TYPE, "struct" },
	{ COLOR_KEYWORD, "switch" },
	{ COLOR_KEYWORD, "true" },
	{ COLOR_TYPE, "typedef" },
	{ COLOR_TYPE, "typeof" },
	{ COLOR_TYPE, "union" },
	{ COLOR_TYPE, "unsigned" },
	{ COLOR_TYPE, "volatile" },
	{ COLOR_KEYWORD, "while" },

	{ COLOR_KEYWORD, "private" },
	{ COLOR_KEYWORD, "public" },
	{ COLOR_KEYWORD, "friend" },
	{ COLOR_KEYWORD, "throw" },
	{ COLOR_KEYWORD, "try" },
	{ COLOR_KEYWORD, "catch" },
	{ COLOR_KEYWORD, "using" },
	{ COLOR_KEYWORD, "virtual" },
	{ COLOR_KEYWORD, "new" },
	{ COLOR_KEYWORD, "delete" },
	{ COLOR_KEYWORD, "namespace" },
	{ COLOR_KEYWORD, "protected" },
	{ COLOR_KEYWORD, "template" },
	{ COLOR_KEYWORD, "operator" },
	{ COLOR_KEYWORD, "class" },
	{ COLOR_KEYWORD, "this" },
	{ COLOR_KEYWORD, "function" },
	{ COLOR_KEYWORD, "fn" },
};

#define C_HASHMAP_SIZE       2048
static i32 _kw_c_table[C_HASHMAP_SIZE];
static hashmap _kw_c =
{
	C_HASHMAP_SIZE, _kw_c_table,
	ARRLEN(_kw_c_keywords), _kw_c_keywords
};

static keyword _kw_asm_6800_keywords[] =
{
	{ COLOR_TYPE, "NOP" },

	{ COLOR_TYPE, "TAP" },
	{ COLOR_TYPE, "TPA" },
	{ COLOR_TYPE, "INX" },
	{ COLOR_TYPE, "DEX" },
	{ COLOR_TYPE, "CLV" },
	{ COLOR_TYPE, "SEV" },
	{ COLOR_TYPE, "CLC" },
	{ COLOR_TYPE, "SEC" },
	{ COLOR_TYPE, "CLI" },
	{ COLOR_TYPE, "SEI" },
	{ COLOR_TYPE, "SBA" },
	{ COLOR_TYPE, "CBA" },

	{ COLOR_TYPE, "TAB" },
	{ COLOR_TYPE, "TBA" },
	{ COLOR_TYPE, "DAA" },

	{ COLOR_TYPE, "ABA" },

	{ COLOR_TYPE, "BRA" },
	{ COLOR_TYPE, "BHI" },
	{ COLOR_TYPE, "BLS" },
	{ COLOR_TYPE, "BLO" },
	{ COLOR_TYPE, "BHS" },
	{ COLOR_TYPE, "BCC" },
	{ COLOR_TYPE, "BCS" },
	{ COLOR_TYPE, "BNE" },
	{ COLOR_TYPE, "BEQ" },
	{ COLOR_TYPE, "BVC" },
	{ COLOR_TYPE, "BVS" },
	{ COLOR_TYPE, "BPL" },
	{ COLOR_TYPE, "BMI" },
	{ COLOR_TYPE, "BGE" },
	{ COLOR_TYPE, "BLT" },
	{ COLOR_TYPE, "BGT" },
	{ COLOR_TYPE, "BLE" },
	{ COLOR_TYPE, "TSX" },
	{ COLOR_TYPE, "INS" },
	{ COLOR_TYPE, "PULA" },
	{ COLOR_TYPE, "PULB" },
	{ COLOR_TYPE, "DES" },
	{ COLOR_TYPE, "TXS" },
	{ COLOR_TYPE, "PSHA" },
	{ COLOR_TYPE, "PSHB" },
	{ COLOR_TYPE, "RTS" },
	{ COLOR_TYPE, "RTI" },
	{ COLOR_TYPE, "WAI" },
	{ COLOR_TYPE, "SWI" },

	{ COLOR_TYPE, "NEGA" },
	{ COLOR_TYPE, "COMA" },
	{ COLOR_TYPE, "LSRA" },
	{ COLOR_TYPE, "RORA" },
	{ COLOR_TYPE, "ASRA" },
	{ COLOR_TYPE, "ASLA" },
	{ COLOR_TYPE, "ROLA" },
	{ COLOR_TYPE, "DECA" },
	{ COLOR_TYPE, "INCA" },
	{ COLOR_TYPE, "TSTA" },
	{ COLOR_TYPE, "CLRA" },

	{ COLOR_TYPE, "NEGB" },
	{ COLOR_TYPE, "COMB" },
	{ COLOR_TYPE, "LSRB" },
	{ COLOR_TYPE, "RORB" },
	{ COLOR_TYPE, "ASRB" },
	{ COLOR_TYPE, "ASLB" },
	{ COLOR_TYPE, "ROLB" },
	{ COLOR_TYPE, "DECB" },
	{ COLOR_TYPE, "INCB" },
	{ COLOR_TYPE, "TSTB" },
	{ COLOR_TYPE, "CLRB" },

	{ COLOR_TYPE, "NEG" },
	{ COLOR_TYPE, "COM" },
	{ COLOR_TYPE, "LSR" },
	{ COLOR_TYPE, "ROR" },
	{ COLOR_TYPE, "ASR" },
	{ COLOR_TYPE, "ASL" },
	{ COLOR_TYPE, "ROL" },
	{ COLOR_TYPE, "DEC" },
	{ COLOR_TYPE, "INC" },
	{ COLOR_TYPE, "TST" },
	{ COLOR_TYPE, "CLR" },

	{ COLOR_TYPE, "JMP" },

	{ COLOR_TYPE, "SUBA" },
	{ COLOR_TYPE, "CMPA" },
	{ COLOR_TYPE, "SBCA" },
	{ COLOR_TYPE, "ANDA" },
	{ COLOR_TYPE, "BITA" },
	{ COLOR_TYPE, "LDAA" },
	{ COLOR_TYPE, "EORA" },
	{ COLOR_TYPE, "ADCA" },
	{ COLOR_TYPE, "ORAA" },
	{ COLOR_TYPE, "ADDA" },
	{ COLOR_TYPE, "CPXA" },
	{ COLOR_TYPE, "STAA" },

	{ COLOR_TYPE, "SUBB" },
	{ COLOR_TYPE, "CMPB" },
	{ COLOR_TYPE, "SBCB" },
	{ COLOR_TYPE, "ANDB" },
	{ COLOR_TYPE, "BITB" },
	{ COLOR_TYPE, "LDAB" },
	{ COLOR_TYPE, "EORB" },
	{ COLOR_TYPE, "ADCB" },
	{ COLOR_TYPE, "ORAB" },
	{ COLOR_TYPE, "ADDB" },
	{ COLOR_TYPE, "STAB" },

	{ COLOR_TYPE, "BSR" },
	{ COLOR_TYPE, "STS" },
	{ COLOR_TYPE, "LDS" },
	{ COLOR_TYPE, "LDX" },
	{ COLOR_TYPE, "STX" },
	{ COLOR_TYPE, "CPX" },
	{ COLOR_TYPE, "JSR" },

	{ COLOR_KEYWORD, "EQU" },
	{ COLOR_KEYWORD, "ORG" },
	{ COLOR_KEYWORD, "DC.W" },
	{ COLOR_KEYWORD, "DC.B" }
};

#define ASM_6800_HASHMAP_SIZE     2048
static i32 _kw_asm_6800_table[ASM_6800_HASHMAP_SIZE];
static hashmap _kw_asm_6800 =
{
	ASM_6800_HASHMAP_SIZE, _kw_asm_6800_table,
	ARRLEN(_kw_asm_6800_keywords), _kw_asm_6800_keywords
};

static keyword _kw_asm_65C02_keywords[] =
{
	{ COLOR_TYPE, "BRK" },
	{ COLOR_TYPE, "ORA" },
	{ COLOR_TYPE, "NOP" },
	{ COLOR_TYPE, "TSB" },
	{ COLOR_TYPE, "ASL" },
	{ COLOR_TYPE, "RMB" },
	{ COLOR_TYPE, "PHP" },
	{ COLOR_TYPE, "BBR" },
	{ COLOR_TYPE, "BPL" },
	{ COLOR_TYPE, "TRB" },
	{ COLOR_TYPE, "CLC" },
	{ COLOR_TYPE, "INC" },
	{ COLOR_TYPE, "JSR" },
	{ COLOR_TYPE, "AND" },
	{ COLOR_TYPE, "BIT" },
	{ COLOR_TYPE, "ROL" },
	{ COLOR_TYPE, "PLP" },
	{ COLOR_TYPE, "BMI" },
	{ COLOR_TYPE, "SEC" },
	{ COLOR_TYPE, "DEC" },
	{ COLOR_TYPE, "RTI" },
	{ COLOR_TYPE, "EOR" },
	{ COLOR_TYPE, "LSR" },
	{ COLOR_TYPE, "PHA" },
	{ COLOR_TYPE, "JMP" },
	{ COLOR_TYPE, "BVC" },
	{ COLOR_TYPE, "CLI" },
	{ COLOR_TYPE, "PHY" },
	{ COLOR_TYPE, "RTS" },
	{ COLOR_TYPE, "ADC" },
	{ COLOR_TYPE, "STZ" },
	{ COLOR_TYPE, "ROR" },
	{ COLOR_TYPE, "PLA" },
	{ COLOR_TYPE, "BVS" },
	{ COLOR_TYPE, "SEI" },
	{ COLOR_TYPE, "PLY" },
	{ COLOR_TYPE, "BRA" },
	{ COLOR_TYPE, "STA" },
	{ COLOR_TYPE, "STY" },
	{ COLOR_TYPE, "STX" },
	{ COLOR_TYPE, "SMB" },
	{ COLOR_TYPE, "DEY" },
	{ COLOR_TYPE, "TXA" },
	{ COLOR_TYPE, "BBS" },
	{ COLOR_TYPE, "BCC" },
	{ COLOR_TYPE, "TYA" },
	{ COLOR_TYPE, "TXS" },
	{ COLOR_TYPE, "LDY" },
	{ COLOR_TYPE, "LDA" },
	{ COLOR_TYPE, "LDX" },
	{ COLOR_TYPE, "TAY" },
	{ COLOR_TYPE, "TAX" },
	{ COLOR_TYPE, "BCS" },
	{ COLOR_TYPE, "CLV" },
	{ COLOR_TYPE, "TSX" },
	{ COLOR_TYPE, "CPY" },
	{ COLOR_TYPE, "CMP" },
	{ COLOR_TYPE, "INY" },
	{ COLOR_TYPE, "DEX" },
	{ COLOR_TYPE, "WAI" },
	{ COLOR_TYPE, "BNE" },
	{ COLOR_TYPE, "CLD" },
	{ COLOR_TYPE, "PHX" },
	{ COLOR_TYPE, "STP" },
	{ COLOR_TYPE, "CPX" },
	{ COLOR_TYPE, "SBC" },
	{ COLOR_TYPE, "INX" },
	{ COLOR_TYPE, "BEQ" },
	{ COLOR_TYPE, "SED" },
	{ COLOR_TYPE, "PLX" },

	{ COLOR_KEYWORD, "EQU" },
	{ COLOR_KEYWORD, "ORG" },
	{ COLOR_KEYWORD, "DW" },
	{ COLOR_KEYWORD, "DB" }
};

#define ASM_65C02_HASHMAP_SIZE     2048
static i32 _kw_asm_65C02_table[ASM_65C02_HASHMAP_SIZE];
static hashmap _kw_asm_65C02 =
{
	ASM_65C02_HASHMAP_SIZE, _kw_asm_65C02_table,
	ARRLEN(_kw_asm_65C02_keywords), _kw_asm_65C02_keywords
};

static u32 kw_hash(char *word, u32 len)
{
	u32 i, c;
	u32 hash = 0x12345678;
	for(i = 0; i < len && (c = word[i]); ++i)
	{
		hash ^= c;
		hash *= 0x5bd1e995;
		hash ^= hash >> 15;
	}

	return hash;
}

static void kw_init(hashmap *hm)
{
	u32 i;
	for(i = 0; i < hm->size; ++i)
	{
		hm->table[i] = -1;
	}

	for(i = 0; i < hm->num_keywords; ++i)
	{
		u32 steps = 0;
		u32 hash = kw_hash(hm->keywords[i].name, UINT32_MAX);
		while(hm->table[hash % hm->size] >= 0)
		{
			++hash;
			assert(++steps < COLLISION_LIMIT);
		}

		hm->table[hash % hm->size] = i;
	}
}

static u32 kw_detect(hashmap *hm, char *str, u32 len)
{
	u32 i;
	u32 hash = kw_hash(str, len);
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

	return COLOR_FG;
}
