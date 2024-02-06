#define TEST(expr)     test((expr), #expr, __FILE__, __LINE__)
#define TEST_FN(name)  test_fn(name, #name)
#define TEST_ALL(name) test_all(name)

#define COLOR_RESET "\033[0m"
#define COLOR_RED   "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE  "\033[1;34m"
#define COLOR_WHITE "\033[1;37m"

static u32 _fn_count, _fn_success, _all_count, _all_success;

static void print_success(u32 success, u32 count)
{
	const char *color = (success == count) ? COLOR_GREEN : COLOR_RED;

	printf("%s> %d / %d tests successful" COLOR_RESET "\n\n",
		color, success, count);
}

static void test(u32 cond, const char *expr, const char *file, u32 line)
{
	++_fn_count;
	++_all_count;
	if(cond)
	{
		++_fn_success;
		++_all_success;
	}
	else
	{
		printf("%s:%d | Assertion failed: %s\n", file, line, expr);
	}
}

static void test_all(void (*fn)(void))
{
	printf(COLOR_BLUE "--- STARTING ALL TESTS ---" COLOR_RESET "\n\n");
	fn();
	printf(COLOR_BLUE "--- ALL TESTS COMPLETED ---" COLOR_RESET "\n");
	print_success(_all_success, _all_count);
}

static void test_fn(void (*fn)(void), const char *name)
{
	printf(COLOR_BLUE "Running:" COLOR_RESET " "
		COLOR_WHITE "%s" COLOR_RESET "\n", name);
	_fn_count = 0;
	_fn_success = 0;
	fn();
	print_success(_fn_success, _fn_count);
}

static void test_dec_digit_cnt(void)
{
	TEST(dec_digit_cnt(99999) == 5);
	TEST(dec_digit_cnt(1) == 1);
	TEST(dec_digit_cnt(12) == 2);
	TEST(dec_digit_cnt(1234) == 4);
	TEST(dec_digit_cnt(100) == 3);
}

static void test_linenr_str(void)
{
	char buf[16];
	linenr_str(buf, 4, 4);
	TEST(!strcmp(buf, "   4"));

	linenr_str(buf, 1000, 4);
	TEST(!strcmp(buf, "1000"));

	linenr_str(buf, 99, 3);
	TEST(!strcmp(buf, " 99"));

	linenr_str(buf, 2, 8);
	TEST(!strcmp(buf, "       2"));

	linenr_str(buf, 1, 1);
	TEST(!strcmp(buf, "1"));
}

static void test_starts_with(void)
{
	TEST(starts_with("hello world", "hello"));
	TEST(starts_with("apple", "app"));
	TEST(starts_with("something", ""));
	TEST(starts_with("what", "what"));
	TEST(starts_with("112", "1"));

	TEST(!starts_with("111", "9"));
	TEST(!starts_with("apfel", "birne"));
	TEST(!starts_with("apple", "appi"));
	TEST(!starts_with("74", "745"));
	TEST(!starts_with("hello", "hello world"));
}

static void test_match_part(void)
{
	TEST(match_part("this is a test", "this", 4));
	TEST(match_part("hello world", "hello", 5));
	TEST(match_part("123456789", "123456", 6));

	TEST(!match_part("volatil", "volatile", 7));
	TEST(!match_part("astatic", "static", 7));
	TEST(!match_part("weird", "weirdness", 5));

	TEST(!match_part("aaa", "bbb", 3));
	TEST(!match_part("aaafa", "aaaga", 5));
}

static void test_list(void)
{
	TEST_FN(test_dec_digit_cnt);
	TEST_FN(test_linenr_str);
	TEST_FN(test_starts_with);
	TEST_FN(test_match_part);
}

static void test_run_all(void)
{
	TEST_ALL(test_list);
}
