typedef enum
{
	RE_MATCH_CASE = 0x01,
	RE_WHOLE_WORD = 0x02
} re_flags;

typedef struct
{
	char *input;
	size_t input_len;

	char *search;
	size_t search_len;

	char *replace;
	size_t replace_len;

	u32 flags;
} re_param;

static void search_replace(re_param *param, vec *result)
{
	vec out;
	vec_init(&out, param->input_len);
	char *p = param->input;
	char *last_part = p;
	char *end = p + param->input_len;
	if(param->search_len <= param->input_len)
	{
		while(p < end)
		{
			if(p + param->search_len > end)
			{
				break;
			}

			if(!memcmp(p, param->search, param->search_len))
			{
				/* Part before replacement */
				vec_push(&out, last_part - p, last_part);

				/* Replacement part */
				vec_push(&out, param->replace_len, param->replace);
				p += param->search_len;
				last_part = p;
			}
			else
			{
				++p;
			}
		}
	}

	/* Last part */
	vec_push(&out, end - last_part, last_part);
	*result = out;
}
