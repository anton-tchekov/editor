#include "test.h"
#include "types.h"

#define COLOR_RESET "\033[0m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE  "\033[1;34m"
#define COLOR_WHITE "\033[1;37m"

static i32 _fn_count, _fn_success, _all_count, _all_success;

static void test_register(void)
{
	++_fn_count;
	++_all_count;
}

static void test_success(void)
{
	++_fn_success;
	++_all_success;
}

static void test_fn_start(const char *name)
{
	printf(COLOR_BLUE "Running:" COLOR_RESET " "
		COLOR_WHITE "%s" COLOR_RESET "\n", name);

	_fn_count = 0;
	_fn_success = 0;
}

static void print_success(i32 success, i32 count)
{
	const char *color = (success == count) ? COLOR_GREEN : COLOR_RED;

	printf("%s> %d / %d tests successful" COLOR_RESET "\n\n",
		color, success, count);
}

static void test_fn_finish(void)
{
	print_success(_fn_success, _fn_count);
}

static void test_all_start(void)
{
	printf(COLOR_BLUE "--- STARTING ALL TESTS ---" COLOR_RESET "\n\n");
}

static void test_all_finish(void)
{
	printf(COLOR_BLUE "--- ALL TESTS COMPLETED ---" COLOR_RESET "\n");
	print_success(_all_success, _all_count);
}

void test(i32 cond, const char *expr, const char *file, i32 line)
{
	test_register();
	if(cond)
	{
		test_success();
	}
	else
	{
		printf("%s:%d | Assertion failed: %s\n", file, line, expr);
	}
}

void test_all(void (*fn)(void))
{
	test_all_start();
	fn();
	test_all_finish();
}

void test_fn(void (*fn)(void), const char *name)
{
	test_fn_start(name);
	fn();
	test_fn_finish();
}
