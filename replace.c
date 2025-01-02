enum
{
	/* Must match values in SR */
	RE_MATCH_CASE = 0x04,
	RE_WHOLE_WORD = 0x08
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

static int re_matches(char *in, re_param *params)
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

static void re_replace_all(re_param *params, vec *result)
{
	vec out = vec_init(params->input_len);
	char *p = params->input;
	char *last_part = p;
	char *end = p + params->input_len;
	if(params->search_len <= params->input_len)
	{
		while(p < end)
		{
			if(p + params->search_len > end)
			{
				break;
			}

			if(re_matches(p, params))
			{
				/* Part before replacement */
				vec_push(&out, p - last_part, last_part);

				/* Replacement part */
				vec_push(&out, params->replace_len, params->replace);

				p += params->search_len;
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
