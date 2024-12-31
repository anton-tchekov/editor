enum
{
	RE_MATCH_CASE = 0x01,
	RE_WHOLE_WORD = 0x02
};

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

static int matches(char *in, re_param *params)
{
	if(params->flags & RE_MATCH_CASE)
	{
		for(size_t i = 0; i < params->search_len; ++i)
		{
			if(in[i] != params->search[i])
			{
				return 0;
			}
		}
	}
	else
	{
		for(size_t i = 0; i < params->search_len; ++i)
		{
			if(tolower(in[i]) != tolower(params->search[i]))
			{
				return 0;
			}
		}
	}

	if(params->flags & RE_WHOLE_WORD)
	{
		u32 prev_c = (in == params->input) ? 0 : in[-1];
		u32 next_c = (in + params->search_len >= params->input + params->input_len) ? 0 : in[params->search_len];
		u32 first_c = in[0];
		u32 last_c = in[params->search_len - 1];

		if(isalpha(first_c) && isalpha(prev_c))
		{
			return 0;
		}

		if(isalpha(last_c) && isalpha(next_c))
		{
			return 0;
		}
	}

	return 1;
}

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

			if(matches(p, param))
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
