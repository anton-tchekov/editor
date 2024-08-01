#include <stdio.h>

int main(void)
{
	int num_lines = 1000;
	int num_cols = 80;

	for(int i = 0; i < num_lines; ++i)
	{
		for(int j = 0; j < num_cols; ++j)
		{
			fputc(32 + (j + i) % 95, stdout);
		}

		fputc('\n', stdout);
	}

	return 0;
}
