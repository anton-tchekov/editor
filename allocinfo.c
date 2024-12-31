#include <malloc.h>

static size_t heap_size_get(void)
{
	struct mallinfo2 mi = mallinfo2();
	return mi.uordblks;
}
