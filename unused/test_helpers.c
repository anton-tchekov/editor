#include "test_helpers.h"
#include "test.h"
#include "helpers.h"

#include <string.h>

void test_dec_digit_cnt(void)
{
	TEST(dec_digit_cnt(99999) == 5);
	TEST(dec_digit_cnt(1) == 1);
	TEST(dec_digit_cnt(12) == 2);
	TEST(dec_digit_cnt(1234) == 4);
	TEST(dec_digit_cnt(100) == 3);
}

void test_linenr_str(void)
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

void test_starts_with(void)
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

void test_match_part(void)
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
