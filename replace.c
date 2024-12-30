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

void search_replace(re_param *param, vec *result)
{
	/* TODO: Finish code */
	char *p, *end;
	vec out;

	vec_init(&out, param->input_len);
	if(param->search_len <= param->input_len)
	{
		p = param->input;
		end = p + param->input_len;
		while(p < end)
		{
			if(p + param->search_len > end)
			{
				break;
			}

			if(matches(p, param->search, param->search_len))
			{
				vec_push(&out, );
				vec_push(&out, );
				p += param->search_len;
			}
			else
			{
				++p;
			}
		}
	}

	/* Last part */
	vec_push(&out, );

	*result = out;
}
