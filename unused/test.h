#ifndef __TEST_H__
#define __TEST_H__

#include <stdio.h>

void test(int cond, const char *expr, const char *file, int line);
void test_all(void (*fn)(void));
void test_fn(void (*fn)(void), const char *name);

#define TEST(expr)     test((expr), #expr, __FILE__, __LINE__)
#define TEST_FN(name)  test_fn(name, #name)
#define TEST_ALL(name) test_all(name)

#endif /* __TEST_H__ */
