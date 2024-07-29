#define TEST(expr)      test((expr), #expr, __FILE__, __LINE__)
#define TEST_FN(name)   test_fn(name, #name)

#define TERM_RESET     "\033[0m"
#define TERM_RED       "\033[1;31m"
#define TERM_GREEN     "\033[1;32m"
#define TERM_BLUE      "\033[1;34m"
#define TERM_WHITE     "\033[1;37m"

static u32 _test_fn_count, _test_fn_success,
	_test_all_count, _test_all_success;

static void print_success(u32 success, u32 count)
{
	printf("%s> %d / %d tests successful" TERM_RESET "\n\n",
		(success == count) ? TERM_GREEN : TERM_RED,
		success, count);
}

static void test(u32 cond, char *expr, char *file, u32 line)
{
	++_test_fn_count;
	++_test_all_count;
	if(cond)
	{
		++_test_fn_success;
		++_test_all_success;
	}
	else
	{
		printf("%s:%d | Assertion failed: %s\n", file, line, expr);
	}
}

static void test_fn(void (*fn)(void), char *name)
{
	printf(TERM_BLUE "Running:" TERM_RESET " "
		TERM_WHITE "%s" TERM_RESET "\n", name);
	_test_fn_count = 0;
	_test_fn_success = 0;
	fn();
	print_success(_test_fn_success, _test_fn_count);
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

static void test_run_all(void)
{
	_test_all_count = 0;
	_test_all_success = 0;

	printf(TERM_BLUE "--- STARTING ALL TESTS ---" TERM_RESET "\n\n");

	TEST_FN(test_dec_digit_cnt);
	TEST_FN(test_linenr_str);
	TEST_FN(test_starts_with);
	TEST_FN(test_match_part);

	printf(TERM_BLUE "--- ALL TESTS COMPLETED ---" TERM_RESET "\n");
	print_success(_test_all_success, _test_all_count);
}
